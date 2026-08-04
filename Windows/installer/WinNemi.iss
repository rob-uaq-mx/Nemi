; WinNemi installer script (Inno Setup 6). Packages the statically-linked
; Release build of WinNemi.exe (see ../CMakeLists.txt's CMAKE_MSVC_RUNTIME_LIBRARY
; setting -- /MT, no VC++ Redistributable dependency) plus the examples/ corpus
; and the bibcom/ common library (v0.2, Nemi.md §19/§22), so a fresh Windows
; machine needs nothing else installed.
;
; Build: "C:\Users\%USERNAME%\AppData\Local\Programs\Inno Setup 6\ISCC.exe" WinNemi.iss
; (or wherever ISCC.exe landed -- see README.md in this folder)
;
; Per-user install (PrivilegesRequired=lowest): no admin rights / UAC prompt
; needed, so it works on locked-down lab/classroom machines.

#define MyAppName "WinNemi"
#define MyAppVersion "0.3.0"
#define MyAppPublisher "Proyecto Nemi"
#define MyAppExeName "WinNemi.exe"
#define RepoRoot "..\..\"
#define ReleaseDir "..\build\Release\"

[Setup]
AppId={{6E9F0C6E-6E7D-4B7E-9C7B-7A9E7B7A5C9D}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
DefaultDirName={localappdata}\Programs\{#MyAppName}
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
PrivilegesRequired=lowest
UninstallDisplayIcon={app}\{#MyAppExeName}
OutputDir=output
OutputBaseFilename=WinNemi-Setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
ArchitecturesInstallIn64BitMode=x64compatible

[Languages]
Name: "spanish"; MessagesFile: "compiler:Languages\Spanish.isl"

[Tasks]
Name: "desktopicon"; Description: "Crear un acceso directo en el escritorio"; GroupDescription: "Accesos directos adicionales:"

[Files]
Source: "{#ReleaseDir}{#MyAppExeName}"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#RepoRoot}examples\*.nemi"; DestDir: "{app}\examples"; Flags: ignoreversion
Source: "{#RepoRoot}bibcom\*.nemi"; DestDir: "{app}\bibcom"; Flags: ignoreversion
Source: "{#RepoRoot}Nemi.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#RepoRoot}docs\manual\*.html"; DestDir: "{app}\manual"; Flags: ignoreversion
Source: "{#RepoRoot}docs\Nemi.html"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#RepoRoot}docs\bibcom\README.html"; DestDir: "{app}\bibcom"; Flags: ignoreversion

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"
Name: "{group}\Desinstalar {#MyAppName}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[INI]
; WinNemi.exe already finds bibcom/ next to itself on its own (see
; WinNemiState.cpp's WinNemiStateImplicitBibcomDir) -- this entry isn't
; needed for that to work. It just seeds the "Rutas de inclusión" dialog
; with one example the first time a student opens it, so the feature (and
; how to add their own folder the same way) is discoverable.
Filename: "{app}\WinNemi.ini"; Section: "Incluye"; Key: "Rutas"; String: "{app}\bibcom"

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "Ejecutar {#MyAppName}"; Flags: nowait postinstall skipifsilent
