
[Setup]
AppName=YouTube Download Manager
AppVersion=1.6
DefaultDirName={pf}\YouTubeDownloadManager
DefaultGroupName=YouTube Download Manager
OutputBaseFilename=YouTubeDownloadManagerInstaller V1.6
Compression=lzma
SolidCompression=yes
WizardStyle=modern

[Tasks]
Name: "startupicon"; Description: "Start program with Windows"; GroupDescription: "Additional icons:"; Flags: unchecked

[Files]
Source: "D:\Projects\YouTubeDownloadManager\build\Desktop_Qt_6_10_1_MSVC2022_64bit-Release\release\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\YouTube Download Manager"; Filename: "{app}\YouTubeDownloadManagerMSVC.exe"

Name: "{userstartup}\YouTube Download Manager"; Filename: "{app}\YouTubeDownloadManagerMSVC.exe"; Tasks: startupicon
