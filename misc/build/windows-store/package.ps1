Param
(
    [Parameter(Mandatory=$true, Position=0)]
    $IdentityNameParam,
    [Parameter(Mandatory=$true, Position=1)]
    $PublisherParam,
    [Parameter(Mandatory=$true, Position=2)]
    $PasswordParam,
    [Parameter(Mandatory=$true, Position=3)]
    $TagParam
)

Copy-Item -Path "misc/build/windows-store/*" -Destination "install" -Recurse -Container -Force
cd "install"

# Generate cert, this is not super sensitive, Microsoft resigns with their own cert
$cert = New-SelfSignedCertificate `
  -Type Custom `
  -Subject $PublisherParam `
  -KeyUsage DigitalSignature `
  -FriendlyName "Xpano" `
  -CertStoreLocation "Cert:\CurrentUser\My" `
  -TextExtension @("2.5.29.37={text}1.3.6.1.5.5.7.3.3", "2.5.29.19={text}")

$password = ConvertTo-SecureString -String $PasswordParam -Force -AsPlainText

Export-PfxCertificate `
  -cert "Cert:\CurrentUser\My\$($cert.Thumbprint)" `
  -FilePath mycert.pfx `
  -Password $password

# Generate app manifest
New-Item -Name "manifests" -ItemType "directory"
jinja `
  -D "XPANO_IDENTITY_NAME" $IdentityNameParam `
  -D "XPANO_PUBLISHER" $PublisherParam `
  -D "XPANO_VERSION" $TagParam `
  Package.appxmanifest.in -o manifests/Package.appxmanifest

# Create resource index files
makepri.exe createconfig /cf priconfig.xml /dq en-US
makepri.exe new /pr . /cf priconfig.xml /mn manifests/Package.appxmanifest

# Add exe manifest
mt.exe -nologo -manifest Xpano.exe.manifest -outputresource:"Xpano.exe;#1"

# Sign binary
signtool sign /fd sha256 /a /f mycert.pfx /p $PasswordParam Xpano.exe

# Make msix bundle
makeappx build /v /f PackagingLayout.xml /op . /ca

# Sign bundle
signtool sign /fd sha256 /a /f mycert.pfx /p $PasswordParam Xpano.msixbundle

cd ..
