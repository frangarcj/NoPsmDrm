# NoPsmDrm Plugin by Frangarcj

## Features
- Exports PSM content license keys as fake licences.
- Bypasses expiration of PSM game licenses.
- Allows sharing PSM content across multiple PS Vita accounts and devices using generated fake license files.

In a nutshell, this plugin allows you to bypass DRM protection on PSM game content.

### This software WILL NOT
- Allow modifications to your games/applications.
- Enable you to run PSM content without a valid license or a fake license file.

### WARNING !!!
- For those with valid PSM activated Vitas and licenses, **BACKUP EVERYTHING inside tm0 and ux0:/psm**  before using this plugin. If there's no backup and some files get removed / erased there is no way to get them back

## Legal Disclaimer
- The removal and distribution of DRM content and/or circumventing copy protection mechanisms for any other purpose than archiving/preserving games you own licenses for is illegal.  
- This software is meant to be strictly reserved for your own **PERSONAL USE**.
- The author does not take any responsibility for your actions using this software.

## Software Requirements
This software will only work on PlayStation Vita, PlayStation Vita TV, PlayStation TV devices running on firmware 3.60, the taiHEN framework and HENkaku need to be running on your device, for more information please connect to https://henkaku.xyz/  

### Installing PSM runtime
When running a game for the first time, it will ask to install psm runtime. In order to download and install it, you need to change your DNS to [Henkaku Update Server](https://www.reddit.com/r/vitahacks/comments/5g819i/henkaku_update_server_easy_and_safe_way_to_update/)

## Installation
Download the latest [nopsmdrm.skprx](https://github.com/frangarcj/NoPsmDrm/releases), copy it to `ux0:tai` and modify the `ux0:tai/config.txt` file to add the path to the module under `*KERNEL` as follows

```
*KERNEL
ux0:tai/nopsmdrm.skprx
```

If you know what you are doing, you may change this path to an arbitrary location as long as it matches the exact location of the module. 
You may also edit the `ur0:tai/config.txt` instead assuming you do not have a config.txt file inside the `ux0:tai/` folder

## Creating the fake license
In order to generate a fake license file containing the application's keys, you must first launch the application with the NoPsmDrm plugin enabled.  
The fake licenses for the applications will then be stored at
- `ux0:data/EM0041-NPOA00013_00-0000000000000000.rif` using content id as filename

## Sharing PSM Games
- If you wish to use the application on the same device but on a different account, simply copy the fake license `ux0:data/EM0041-NPOA00013_00-0000000000000000.rif` to
  `ux0:psm/NPOA00013/RO/License/FAKE.rif`.
- If you wish to use the application on a different device, transfer the content of `ux0:psm/TITLE_ID` to your PC and copy the fake license `ux0:data/EM0041-NPOA00013_00-0000000000000000.rif` file as `ux0:psm/TITLE_ID/RO/License/FAKE.rif` **You need to update / rebuild database**

## Installing shared games
- PSM games must be stored at the following location: `ux0:psm/TITLE_ID`
- You must update / rebuild database, for example by removing `ux0:id.dat` and rebooting. A dialogue my pop up asking if  a data transfer should be performed. Press "No", otherwise the memory card will be formatted.

## Source code
The source code is located within the `src` directory and is licensed under `GPLv3`.

## Donation
All my work is voluntary and nonprofit, however you can make children happy by making a small donation to [Fundación Juegaterapia](https://www.juegaterapia.org/?lang=en). Let me know if you donate something. Thanks!!!

**Note**:  I'm not affiliated, sponsored, or otherwise endorsed by Juegaterapia. I just like their work.

## Special thanks
- Thanks to Team molecule ([Davee](https://twitter.com/DaveeFTW), Proxima, [xyz](https://twitter.com/pomfpomfpomf3), [yifanlu](https://twitter.com/yifanlu)) for HENkaku, taiHEN and everything else they have done for the scene.
- Thanks to [Motoharu](https://github.com/motoharu-gosuto) for all his work on NpDrm.
- Thanks to [CodeRobe](https://twitter.com/coderobe), [Devnoname120](https://twitter.com/devnoname120) and [Rinnegatamante](https://twitter.com/Rinnegatamante) for supporting me during beta testing.
- Thanks to [TheFlow](https://twitter.com/theflow0) for [NoNpDrm](https://github.com/TheOfficialFloW/NoNpDrm)
- Thanks to everyone that helped at vitahacks [thread1](https://www.reddit.com/r/vitahacks/comments/71xuq9/nopsmdrm_status_and_help/),[thread2](https://www.reddit.com/r/vitahacks/comments/6cqokl/some_research_on_psm_games_do_you_still_have_some/)
