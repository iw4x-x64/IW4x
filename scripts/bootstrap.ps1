& "bdep" "deinit" "--force"
& "bdep" "config" "remove" "-@msvc"
Remove-Item -Recurse -Force D:\IW64x\iw4x-msvc\
Remove-Item -Recurse -Force D:\IW64x\iw4x-host\
& "bdep" init -C -@msvc cc config.cxx=cl config.cxx.coptions=/Z7 config.cxx.loptions=/DEBUG:FULL config.install.root=D:\IW64x\english config.install.bin=D:\IW64x\english config.install.filter='include/@false lib/@false share/@false'

# [CmdletBinding (SupportsShouldProcess = $true)]
# param (
#   [Parameter (HelpMessage = "Display this help message.")]
#   [switch] $ShowHelp = $False,

#   [Parameter (HelpMessage = "Remove existing bdep state and generated host configurations before initialization.")]
#   [switch] $Recreate = $False,

#   [Parameter (HelpMessage = "Path to IW4x installation root.",
#     Mandatory = $true,
#     Position = 0,
#     ParameterSetName = "LiteralPath",
#     ValueFromPipelineByPropertyName = $true)]
#   [Alias("PSPath")]
#   [ValidateNotNullOrEmpty()]
#   [string] $LiteralPath
# )

# $ErrorActionPreference = "Stop"
# $ProgressPreference = "Continue"

# if (-not (Get-Command bdep)) {
#   throw "bdep not found in PATH"
  
# }

# # Derive project directory basename.
# #
# $project = Split-Path -Parent $PSScriptRoot

# # Cache path for IW4x installation root.
# #
# cache="${XDG_CACHE_HOME:-$HOME/.cache}/iw4x/bootstrap-root"

# # Recreate mode: remove local build2 state.
# #
# if ($Recreate) {
#   Write-Host "[+] Recreating configurations: removing existing state..."

#   $bdepDir = Join-Path $PSScriptRoot ".bdep"
#   $hostDir = Join-Path  "$project-host"

#   if (Test-Path $bdepDir) {
#     Remove-Item -LiteralPath $bdepDir -Recurse -Force
#   }

#   if (Test-Path $bdepDir) {
#     Remove-Item -LiteralPath $hostDir -Recurse -Force
#   }
# }