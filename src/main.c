/*
    NoPsmDrm Plugin
    Copyright (C) 2017, frangarcj

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <psp2kern/kernel/modulemgr.h>
#include <psp2kern/kernel/sysmem.h>
#include <psp2kern/kernel/threadmgr.h>
#include <psp2kern/registrymgr.h>
#include <psp2kern/io/dirent.h>
#include <psp2kern/io/fcntl.h>
#include <psp2kern/io/stat.h>
#include <stdio.h>
#include <string.h>

#include <taihen.h>

#define FAKE_AID 0x0123456789ABCDEFLL
#define UNACTIVATED_AID 0x21191d6d5de7c6dbLL

static tai_hook_ref_t ksceKernelLaunchAppRef;
static tai_hook_ref_t ScePsmDrmForDriver_B09003A7_ref;
static tai_hook_ref_t ScePsmDrmForDriver_984F9017_ref;
static tai_hook_ref_t ScePsmDrmForDriver_8C8CFD01_ref;

static SceUID hooks[4];
static int n_hooks = 0;

typedef struct
{
    char magic[0x8];             // 0x00
    uint32_t unk1;               // 0x08
    uint32_t unk2;               // 0x0C
    uint64_t aid;                // 0x10
    uint32_t unk3;               // 0x18
    uint32_t unk4;               // 0x1C
    uint64_t start_time;         // 0x20
    uint64_t expiration_time;    // 0x28
    uint8_t act_digest[0x20];    // 0x30
    char content_id[0x30];       // 0x50
    uint8_t unk5[0x80];          // 0x80
    uint8_t key[0x200];          // 0x100
    uint8_t sha256digest[0x100]; // 0x300
} ScePsmDrmLicense;

int ksceNpDrmGetRifName(char *rif_name, uint32_t flags, uint64_t aid);

void debug_printf(char *msg)
{
#ifdef DEBUG
    SceUID fd = ksceIoOpen("ux0:data/nopsmdrm.txt", SCE_O_WRONLY | SCE_O_CREAT | SCE_O_APPEND, 0777);
    if (fd >= 0)
    {
        ksceIoWrite(fd, msg, strlen(msg));
        ksceIoClose(fd);
    }
#endif
}

#define debugPrintf(...)                         \
    \
{                                         \
        char msg[128];                           \
        snprintf(msg, sizeof(msg), __VA_ARGS__); \
        debug_printf(msg);                       \
    \
}

char reg_buffer[2048];

int get_account_id(uint64_t *aid)
{

    memset(reg_buffer, 0, sizeof(reg_buffer));

    int reg_res = ksceRegMgrGetKeyBin("/CONFIG/NP/", "account_id", reg_buffer, sizeof(reg_buffer));
    if (reg_res < 0)
    {
        debugPrintf("failed to get AID: %x\n", reg_res);
        return -1;
    }

    debugPrintf("Got PSN AID\n");

    memcpy(aid, reg_buffer, 0x8);

    if (*aid == UNACTIVATED_AID)
        *aid = FAKE_AID;

    return 0;
}

int FindLicenses(char *path)
{

    char rif_name[48];
    char new_path[256];
    uint64_t aid;

    int real_rif_exists = 0;
    SceUID dfd = ksceIoDopen(path);
    if (dfd < 0)
        return dfd;

    int res = 0;

    do
    {
        SceIoDirent dir;
        memset(&dir, 0, sizeof(SceIoDirent));

        res = ksceIoDread(dfd, &dir);
        if (res > 0)
        {
            strncpy(new_path, path, sizeof(new_path) - 1);
            strncat(new_path, "/", sizeof(new_path) - 1);
            strncat(new_path, dir.d_name, sizeof(new_path) - 1);

            if (!SCE_S_ISDIR(dir.d_stat.st_mode))
            {
                SceUID fd = ksceIoOpen(new_path, SCE_O_RDONLY, 0);
                if (fd >= 0)
                {
                    ScePsmDrmLicense license;
                    int size = ksceIoRead(fd, &license, sizeof(ScePsmDrmLicense));
                    ksceIoClose(fd);

                    if (size == sizeof(ScePsmDrmLicense) && license.aid != FAKE_AID)
                    {
                        real_rif_exists = 1;
                        break;
                    }
                }
            }
        }
    } while (res > 0 && !real_rif_exists);

    ksceIoDclose(dfd);

    get_account_id(&aid);
    ksceNpDrmGetRifName(rif_name, 0, aid);
    debugPrintf("ksceNpDrmGetRifName: %s\n", rif_name);
    if (real_rif_exists && strstr(new_path, rif_name))
    {
        debugPrintf("REAL RIF at: %s\n", new_path);
        return 0;
    }

    snprintf(new_path, sizeof(new_path) - 1, "%s/FAKE.rif", path);
    SceUID fd = ksceIoOpen(new_path, SCE_O_RDONLY, 0);
    debugPrintf("Copy FAKE RIF at: %s %x\n", new_path, fd);
    if (fd >= 0)
    {
        ScePsmDrmLicense license;
        ksceIoRead(fd, &license, sizeof(ScePsmDrmLicense));
        ksceIoClose(fd);

        snprintf(new_path, sizeof(new_path) - 1, "%s/%s", path, rif_name);

        fd = ksceIoOpen(new_path, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
        debugPrintf("To FAKE RIF at: %s %x\n", new_path, fd);
        if (fd < 0)
            return fd;
        ksceIoWrite(fd, &license, sizeof(ScePsmDrmLicense));
        ksceIoClose(fd);
    }
    else
    {
        debugPrintf("FAKE RIF not found at %s\n", new_path);
    }

    return 0;
}

static SceUID _ksceKernelLaunchAppPatched(void *args)
{
    char *titleid = (char *)((uintptr_t *)args)[0];
    uint32_t flags = (uint32_t)((uintptr_t *)args)[1];
    char *path = (char *)((uintptr_t *)args)[2];
    void *unk = (void *)((uintptr_t *)args)[3];

    if (flags == 0x1000000 && strstr(path, "PCSI00011"))
    {
        char license_path[256];

        debugPrintf("titleid: %s\n", titleid);
        debugPrintf("flags: 0x%lx\n", flags);
        debugPrintf("path: %s\n", path);
	
	ksceRegMgrSetKeyInt("/CONFIG/PSM/", "revocation_check_req", 0); //set revocation_check_req to 0. 

        snprintf(license_path, sizeof(license_path) - 1, "ux0:psm/%s/RO/License", titleid);
        debugPrintf("license_path: %s\n", license_path);
        FindLicenses(license_path);
    }

    return TAI_CONTINUE(int, ksceKernelLaunchAppRef, titleid, flags, path, unk); // returns pid
}

static SceUID ksceKernelLaunchAppPatched(char *titleid, uint32_t flags, char *path, void *unk)
{
    uintptr_t args[4];
    args[0] = (uintptr_t)titleid;
    args[1] = (uintptr_t)flags;
    args[2] = (uintptr_t)path;
    args[3] = (uintptr_t)unk;

    return ksceKernelRunWithStack(0x4000, _ksceKernelLaunchAppPatched, args);
}

static int ScePsmDrmForDriver_B09003A7_patched(uint32_t *unk1A, uint32_t *unk1B, uint64_t *aid, uint64_t *start_time, uint64_t *expiration_time)
{
    int res = TAI_CONTINUE(int, ScePsmDrmForDriver_B09003A7_ref, unk1A, unk1B, aid,
                           start_time, expiration_time);

    debugPrintf("ScePsmDrmForDriver_B09003A7_ref: %x\n", res);

    // Bypass expiration time for PSM account
    if (start_time)
        *start_time = 0LL;
    if (expiration_time)
        *expiration_time = 0x7FFFFFFFFFFFFFFFLL;

    // Get fake rif info and return success
    if (aid)
    {
        *aid = 0LL;
        get_account_id(aid);
        debugPrintf("AID: %llx\n", *aid);
    }

    return 0;
}

static int _ScePsmDrmForDriver_8C8CFD01_patched(void *args)
{

    ScePsmDrmLicense *license_buf = (ScePsmDrmLicense *)((uintptr_t *)args)[0];
    uint8_t *klicensee = (uint8_t *)((uintptr_t *)args)[1];
    uint32_t *flags = (uint32_t *)((uintptr_t *)args)[2];
    uint64_t *start_time = (uint64_t *)((uintptr_t *)args)[3];
    uint64_t *expiration_time = (uint64_t *)((uintptr_t *)args)[4];

    int res;
    char path[256];
    //char rif_name[48];

    res = TAI_CONTINUE(int, ScePsmDrmForDriver_8C8CFD01_ref, license_buf, klicensee, flags,
                       start_time, expiration_time);

    debugPrintf("ScePsmDrmForDriver_8C8CFD01_ref: %x\n", res);

    if (res == 0)
    {

        snprintf(path, sizeof(path) - 1, "ux0:/data/%s.rif", license_buf->content_id);

        // Make license structure
        ScePsmDrmLicense license;
        memset(&license, 0, sizeof(ScePsmDrmLicense));
        license.aid = FAKE_AID;
        license.unk1 = __builtin_bswap32(1);

        debugPrintf("content_id: %s\n", license_buf->content_id);

        memcpy(license.content_id, license_buf->content_id, 0x30);
        memcpy(license.key, klicensee, 0x200);

        // Write fake license
        char *c;
        for (c = path; *c; c++)
        {
            if (*c == '/')
            {
                *c = '\0';
                res = ksceIoMkdir(path, 0777);
                debugPrintf("ksceIoMkdir: %s %x\n", path, res);
                *c = '/';
            }
        }

        SceUID fd = ksceIoOpen(path, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
        debugPrintf("ksceIoOpen: %s %x\n", path, fd);
        if (fd < 0)
            return fd;

        ksceIoWrite(fd, &license, sizeof(ScePsmDrmLicense));
        ksceIoClose(fd);

        return 0;
    }
    else
    {
        // Bypass expiration time for PSM games
        if (start_time)
            *start_time = 0LL;
        if (expiration_time)
            *expiration_time = 0x7FFFFFFFFFFFFFFFLL;

        // Get fake rif info and klicensee and return success
        if (license_buf && license_buf->aid == FAKE_AID)
        {
            debugPrintf("faking it\n");
            if (klicensee)
            {
                memcpy(klicensee, license_buf->key, 0x200);
                debugPrintf("with license\n");
            }

            return 0;
        }

        return res;
    }
}

static int ScePsmDrmForDriver_8C8CFD01_patched(ScePsmDrmLicense *license_buf, uint8_t *klicensee, uint32_t *flags, uint64_t *start_time, uint64_t *expiration_time)
{
    uintptr_t args[5];
    args[0] = (uintptr_t)license_buf;
    args[1] = (uintptr_t)klicensee;
    args[2] = (uintptr_t)flags;
    args[3] = (uintptr_t)start_time;
    args[4] = (uintptr_t)expiration_time;

    return ksceKernelRunWithStack(0x4000, _ScePsmDrmForDriver_8C8CFD01_patched, args);
}

static int ScePsmDrmForDriver_984F9017_patched(ScePsmDrmLicense *license_buf, char *content_id, uint64_t *aid, uint64_t *start_time, uint64_t *expiration_time)
{
    int res = TAI_CONTINUE(int, ScePsmDrmForDriver_984F9017_ref, license_buf, content_id, aid,
                           start_time, expiration_time);

    debugPrintf("ScePsmDrmForDriver_984F9017_patched: %x\n", res);
    // Bypass expiration time for PSM games
    if (start_time)
        *start_time = 0LL;
    if (expiration_time)
        *expiration_time = 0x7FFFFFFFFFFFFFFFLL;

    // Get fake rif info and return success
    if (res < 0 && license_buf && license_buf->aid == FAKE_AID)
    {
        if (content_id)
            memcpy(content_id, license_buf->content_id, 0x30);
        if (aid)
        {
            *aid = 0LL;
            get_account_id(aid);
            debugPrintf("AID: %llx\n", *aid);
        }
        return 0;
    }

    return res;
}

void _start() __attribute__((weak, alias("module_start")));
int module_start(SceSize args, void *argp)
{

    hooks[n_hooks++] = taiHookFunctionExportForKernel(KERNEL_PID, &ksceKernelLaunchAppRef, "SceProcessmgr", 0x7A69DE86, 0x71CF71FD, ksceKernelLaunchAppPatched);
    hooks[n_hooks++] = taiHookFunctionExportForKernel(KERNEL_PID, &ScePsmDrmForDriver_B09003A7_ref, "SceNpDrm", 0x9F4924F2, 0xB09003A7, ScePsmDrmForDriver_B09003A7_patched);
    hooks[n_hooks++] = taiHookFunctionExportForKernel(KERNEL_PID, &ScePsmDrmForDriver_984F9017_ref, "SceNpDrm", 0x9F4924F2, 0x984F9017, ScePsmDrmForDriver_984F9017_patched);
    hooks[n_hooks++] = taiHookFunctionExportForKernel(KERNEL_PID, &ScePsmDrmForDriver_8C8CFD01_ref, "SceNpDrm", 0x9F4924F2, 0x8C8CFD01, ScePsmDrmForDriver_8C8CFD01_patched);

    return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize args, void *argp)
{

    if (hooks[--n_hooks] >= 0)
        taiHookReleaseForKernel(hooks[n_hooks], ScePsmDrmForDriver_8C8CFD01_ref);
    if (hooks[--n_hooks] >= 0)
        taiHookReleaseForKernel(hooks[n_hooks], ScePsmDrmForDriver_984F9017_ref);
    if (hooks[--n_hooks] >= 0)
        taiHookReleaseForKernel(hooks[n_hooks], ScePsmDrmForDriver_B09003A7_ref);
    if (hooks[--n_hooks] >= 0)
        taiHookReleaseForKernel(hooks[n_hooks], ksceKernelLaunchAppRef);

    return SCE_KERNEL_STOP_SUCCESS;
}
