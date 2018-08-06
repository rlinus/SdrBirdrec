#to allow script excecution run in powershell:
#Set-ExecutionPolicy -Scope CurrentUser Unrestricted

#run as admin
if (!([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole] "Administrator")) { Start-Process powershell.exe "-NoProfile -ExecutionPolicy Bypass -File `"$PSCommandPath`"" -Verb RunAs; exit }

[Environment]::SetEnvironmentVariable("UHD_IMAGES_DIR", "$PSScriptRoot\bin\SoapySdr\images", "Machine")