[Setup]
AppName=Blitz-NPC Synthesizer
AppVersion=2.0.0
DefaultDirName={commoncf64}\VST3
DisableDirPage=yes
OutputDir=..\dist
OutputBaseFilename=Blitz-NPC_Windows_Setup
Compression=lzma2
SolidCompression=yes
ArchitecturesInstallIn64BitMode=x64compatible

[Files]
Source: "..\..\build\BlitzNPC_artefacts\Release\VST3\Blitz-NPC.vst3\*"; DestDir: "{commoncf64}\VST3\Blitz-NPC.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\assets\Banks\*"; DestDir: "{userdocs}\Blitz-NPC\Banks"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\assets\Skins\*"; DestDir: "{userdocs}\Blitz-NPC\Skins"; Flags: ignoreversion recursesubdirs createallsubdirs
