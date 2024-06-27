# AVPStudio
Dolby Cinema signature entrance (AVP) display content production tool.

[简体中文](/README_zh_CN.md)

![GitHub Downloads (all assets, all releases)](https://img.shields.io/github/downloads/izwb003/AVPStudio/total)
![GitHub Release](https://img.shields.io/github/v/release/izwb003/AVPStudio)
![GitHub last commit](https://img.shields.io/github/last-commit/izwb003/AVPStudio)
![Follow bilibili](https://img.shields.io/badge/follow_me-on_bilibili-blue?logo=bilibili&link=https%3A%2F%2Fspace.bilibili.com%2F36937211)
![GitHub Repo stars](https://img.shields.io/github/stars/izwb003/AVPStudio)


## Introduction
Some Dolby Cinemas have an AVP system built at the entrance of the cinema. This system uses Christie PandorasBox®, a technology developed by Christie Digital® which use multiple ultra short focus projectors to project multiple adjacent images, perform geometric correction and edge fusion, and present an immersive long image on a white wall to create a viewing atmosphere for the audience before entering the theater.

Making contents for Christie® PandorasBox® system requires the use of professional content creation software and related tools developed by Christie Digital. Unfortunately, these tools are expensive and have a high authorization threshold, making them unsuitable for DIY purposes such as movie watching activities. However, the PandorasBox system used by Dolby Cinema® has preset a screen separation template, and we only need to refer to this template to choose a special arrangement specification video and add it to PandorasBox® timeline to generate an immersive display effect similar to Dolby's official content.

AVPStudio is an encoding tool. It can rearrange and encode any video into a PandorasBox® playable video according to the preset separation template format supported by the system, enables the playback of self-made AVP contents, which can be applied in situations where self-made content needs to be projected during activities.

## Instructions
**Note: This program is only applicable to Dolby AVP V2. Other types of configurations have not been tested. AVPStudio does not guarantee availability for other types of configurations.**

### I'm not familiar with programming. Where should I download and run it?
Please visit [this link](https://github.com/izwb003/AVPStudio/releases/latest) to download the latest version of executable programs and install them into your computer.

### Video preparation
Dolby Cinema®'s AVPs have three different specifications. The specific parameters for the three specifications are as follows:
| Spec   | Size        | Resolution    | Audio Track      |
| ------ | ----------- | ------------- | ---------------- |
| Small  | 5.5m x 2.1m | 2830W x 1080H | 7.1 PCM Surround |
| Medium | 9m x 2.1m   | 4633W x 1080H | 7.1 PCM Surround |
| Large  | 12m x 2.1m  | 6167W x 1080H | 7.1 PCM Surround |

For specific specifications of the target cinema, please consult the cinema staff. If they are unable to disclose accurate information, you can measure the actual projection area by yourself.

> Hint:
> You can learn about the specification of the AVP by consulting the official video/audio file sent by Dolby to the cinema. For V2 configuration, the file naming convention is as follows:
> V2_Name_audio_all.wav
> V2_Name_video_Xm.mxl
> X refers to the width of the AVP of this cinema.

AVPStudio also provides a reference table that provides configuration specifications for some known cinemas.

After confirming the specifications, please create the content video at the resolution of the target specification. Be sure to pay attention to:

- According to the default configuration, the video will automatically loop through. Therefore, please consider the transition from beginning to end.
- The length of the film cannot be too long.
- Please create common standard SDR videos and control the bit rate within a reasonable range. Suggest considering using CBR. Do not attempt to create high specification content as it may face compatibility issues.
- The maximum audio channel is 7.1 channels. 5.1 or stereo will also be supported.
- Try to lower the audio volume as much as possible. Otherwise, it will affect the viewing experience in the cinema.
- The standard generally requires video width or height to be even. Therefore, the video you edited may have an even resolution (4632x1080 or 6166x1080), which is normal and can be processed correctly by AVPStudio.

### Conversion
- Start AVPStudio, select the corridor size of your target cinema, and click "Create".
- Drag and drop the video you have created into the program window, or click "browse" to select the video file.
- Play and preview the screen effect, and confirm the necessary settings in the "Output Settings" on the right.
- Click "Export" and wait for the program to finish running.
- Contact the engineer of the cinema to copy the generated video files and audio files into the PandorasBox® system in the cinema and drag the content onto the timeline using the same method as adding official Dolby content.

### What should I do if my content automatically jumps back to the beginning before it finishes playing?
Please pay attention to PandorasBox®'s timeline, pay attention to the two "cue" markers highlighted in the picture:

![](images/pandorasbox_timeline_mark_hint.jpg)

Your video will be played from "![](images/pandorasbox_start_mark.png)" mark and ends at "![](images/pandorasbox_cue_mark.png)" mark, then jump back to "![](images/pandorasbox_start_mark.png)" mark to form a loop. You can click and drag the "![](images/pandorasbox_cue_mark.png)" mark to adjust its position like following:

![](images/pandorasbox_drag_cue.jpg)

Thus, by dragging "![](images/pandorasbox_cue_mark.png)" mark to a further position (even the end of the video), you can make the loop (cue) to be longer in order to avoid jumping back to the beginning while playing.

## Additional Tools

### AVPStudio ImageOrganizer
A picture tool used to construct images that conform to the AVP separation templete of Dolby Cinema®.

If your screening content is only a single static image, this tool can save the time of creating videos.

### AVPStudio WAVGenerator
A tool for generating audio WAV file.

It can work together with ImageOrganizer to add background musics for image contents, and also can be used to adjust the volume of the WAV audio.

### AVPStudio MXLPlayer
MXL file player.

A tool for playing converted (or official) mxl file to preview, and also can convert mxl file into H264 mp4 videos.

## Technical Information

### Principle explaination
For specific principles and screen structure of the implementation, please refer to [this document](https://www.bilibili.com/read/cv27334455/) (written with Simplified Chinese).

### Construct and compile note
As of now, the software has only been debugged and tested under a Windows environment, and has not yet been configured and debugged for Linux and macOS environments.

CMake scripts have been adjusted to download pre built ffmpeg from the Internet by default. Please ensure that the Internet connection is unblocked when building. You can also refer to CMakeLists.txt to configure external libraries yourself.

Building requires a complete Qt6 environment. The project must use the following Qt libraries: Qt6Core, Qt6Widgets, Qt6Multimedia, Qt6MultimediaWidgets, Qt6Network.

## Acknowledgements and Announcements
The birth of AVPStudio cannot be separated from [@筱理_Rize](https://space.bilibili.com/3848521/)'s exploration results. All implementation principles of this software have been derived by @筱理_Rize through communication, self testing, and experience.

AVPStudio needs to thank [@冷小鸢aque](https://space.bilibili.com/27063907/) and [@讓晚風温暖各位的心](https://space.bilibili.com/122957742/) for the testing conclusions drawn in practice.

For more acknowledgements, please refer to the software's ["About"](res/texts/aboutinfo_zh_CN.md) page.

Dolby®、Dolby Cinema® are registered trademarks of Dolby Laboratories.

Christie®、Christie Pandoras Box® are registered trademarks of Christie Digital.

All other trademarks are the property of their respective owners.

AVPStudio is not related to Dolby Laboratories or Christie Digital Systems. The output of AVPStudio cannot represent the product quality of the aforementioned enterprises. AVPStudio is only designed for UGC content creation purposes and cannot be used for professional content distribution work. For professional content distribution needs, please contact Dolby Laboratories or Christie Digital.

According to GNU General Public License, terms 11 and 12, because the program is licensed free of charge, there is no warranty for the program, to the extent permitted by applicable law. Except when otherwise stated in writing the copyright holders and/or other parties provide the program "as is" without warranty of any kind, either expressed or implied, including, but not limited to, the implied warranties of merchantability and fitness for a particular purpose. The entire risk as to the quality and performance of the program is with you. Should the
program prove defective, you assume the cost of all necessary servicing, repair or correction. In no event unless required by applicable law or agreed to in writing
will any copyright holder, or any other party who may modify and/or redistribute the program as permitted above, be liable to you for damages, including any general, special, incidental or consequential damages arising out of the use or inability to use the program (including but not limited to loss of data or data being rendered inaccurate or losses sustained by you or third parties or a failure of the program to operate with any other programs), even if such holder or other party has been advised of the possibility of such damages.

## Third-party Software and Open Source Licenses

AVPStudio uses Qt6 technology under the Qt license.

AVPStudio is based on LGPLv2.1 and GPLv2 using [FFmpeg](https://ffmpeg.org/)'s software.

AVPStudio MXLPlayer uses zlib license from [SDL](https://www.libsdl.org/)'s software.

AVPStudio is an open-source software under [GNU GPLv2](https://www.gnu.org/licenses/old-licenses/gpl-2.0.html#SEC1).