[Setup]
AppName=Zynk
AppVersion=1.6.5
AppPublisher=Guille
DefaultDirName={localappdata}\Zynk
DefaultGroupName=Zynk
OutputDir=installer_output
OutputBaseFilename=Zynk_Setup_v1.0
Compression=lzma2
SolidCompression=yes

PrivilegesRequired=lowest

[Files]
Source: "release\zynk.exe"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\Zynk"; Filename: "{app}\zynk.exe"


[Code]
procedure CurStepChanged(CurStep: TSetupStep);
var
  OldPath: String;
  NewPath: String;
  AppDir: String;
begin
  if CurStep = ssPostInstall then
  begin
    AppDir := ExpandConstant('{app}');
    
    
    if RegQueryStringValue(HKEY_CURRENT_USER, 'Environment', 'Path', OldPath) then
    begin
      if Pos(';' + AppDir + ';', ';' + OldPath + ';') = 0 then
      begin
        NewPath := OldPath + ';' + AppDir;
        RegWriteStringValue(HKEY_CURRENT_USER, 'Environment', 'Path', NewPath);
      end;
    end
    else
    begin
      
      RegWriteStringValue(HKEY_CURRENT_USER, 'Environment', 'Path', AppDir);
    end;
  end;
end;
