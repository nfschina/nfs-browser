Name: nfs-browser

Version: 5.2.15

Release: 2666%{?dist}

Summary: compiled from 5.2-15 by wqx

Group: System Environment/Daemons

License: GPL

URL: http://www.nfs.com

Source0: nfs-browser-5.2.15.tar.xz
Patch0:  0001.patch
Patch1:  0002-disable-plugin-install-info-bar.patch
Patch2:  0003-disable-plugin-install-info-bar.patch
Patch3:  0004-change-version-2665.patch
Patch4:  0005-change-mode-of-pepperflash.patch

BuildRoot: %(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)

BuildRequires: make, gcc, python2-ptyprocess, redhat-lsb-core, bison, gperf, libXScrnSaver
Requires: python2-ptyprocess, redhat-lsb-core, bison, gperf, libXScrnSaver, libcanberra-gtk2

%define INSTALL_DIR %{buildroot}/opt/nfs-browser
%define OUT_DEFAULT_DIR out/Release
%define CUR_VERSION 5.2.15.2666

%description

nfs-browser. Compiled from 5.2.15 by wqx

%prep

%setup -q

%patch0 -p1
%patch1 -p1
%patch2 -p1
%patch3 -p1
%patch4 -p1

%define debug_package %{nil}

%build
mkdir -p depot_tools
unzip -qo depot_tools.zip -d depot_tools
ln -s /usr/bin/python2 depot_tools/python
export PATH=$PATH:`pwd`/depot_tools
python2 tools/install_tools.py
alias python="python2"
gn gen out/Release --args='is_component_build=false is_debug=false enable_linux_installer=false enable_nacl=false enable_nacl_nonsfi=false ffmpeg_branding="Chrome" proprietary_codecs=true safe_browsing_mode=0 is_official_build=false is_development_mode=false is_component_ffmpeg=true' --script-executable='/usr/bin/python2'

ninja -C out/Release nfs-browser cdos-cryptor nfsbrowser_sandbox
cp out/Release/libnfs_browser.so ../nfs-browser.log
strip out/Release/nfs-browser
strip out/Release/libblink_core.so
strip out/Release/libnfs_browser.so
strip out/Release/libv8.so
strip out/Release/libcurl.so
strip out/Release/libffmpeg.so

%install
mkdir -p %{buildroot}/usr/share/applications
mkdir -p %{buildroot}/usr/bin
mkdir -p %{INSTALL_DIR}/%{CUR_VERSION}/locales
mkdir -p %{INSTALL_DIR}/%{CUR_VERSION}/Murl
mkdir -p %{INSTALL_DIR}/%{CUR_VERSION}/extensions
mkdir -p %{INSTALL_DIR}/lib
mkdir -p %{INSTALL_DIR}/share

cp debian/nfs-browser.desktop %{buildroot}/usr/share/applications
cp -r debian/icons %{INSTALL_DIR}
cp %{OUT_DEFAULT_DIR}/nfs-browser-launcher %{INSTALL_DIR}/nfs-browser
cp %{OUT_DEFAULT_DIR}/nfsbrowser_sandbox %{INSTALL_DIR}/nfsbrowser-sandbox
cp %{OUT_DEFAULT_DIR}/nfs-browser-up %{INSTALL_DIR}/nfs-browser-up
cp %{OUT_DEFAULT_DIR}/nfs-browser %{INSTALL_DIR}/%{CUR_VERSION}
cp %{OUT_DEFAULT_DIR}/libblink_core.so %{INSTALL_DIR}/%{CUR_VERSION}
cp %{OUT_DEFAULT_DIR}/libnfs_browser.so %{INSTALL_DIR}/%{CUR_VERSION}
cp %{OUT_DEFAULT_DIR}/libv8.so %{INSTALL_DIR}/%{CUR_VERSION}
cp %{OUT_DEFAULT_DIR}/libcurl.so %{INSTALL_DIR}/%{CUR_VERSION}
cp %{OUT_DEFAULT_DIR}/libffmpeg.so %{INSTALL_DIR}/%{CUR_VERSION}
cp %{OUT_DEFAULT_DIR}/natives_blob.bin %{INSTALL_DIR}/%{CUR_VERSION}
cp %{OUT_DEFAULT_DIR}/snapshot_blob.bin %{INSTALL_DIR}/%{CUR_VERSION}
cp %{OUT_DEFAULT_DIR}/xdg-mime %{INSTALL_DIR}/%{CUR_VERSION}
cp %{OUT_DEFAULT_DIR}/xdg-settings %{INSTALL_DIR}/%{CUR_VERSION}
cp %{OUT_DEFAULT_DIR}/browser_100_percent.pak %{INSTALL_DIR}/%{CUR_VERSION}
cp %{OUT_DEFAULT_DIR}/browser_200_percent.pak %{INSTALL_DIR}/%{CUR_VERSION}
cp %{OUT_DEFAULT_DIR}/resources.pak %{INSTALL_DIR}/%{CUR_VERSION}
cp %{OUT_DEFAULT_DIR}/icudtl.dat %{INSTALL_DIR}/%{CUR_VERSION}
cp %{OUT_DEFAULT_DIR}/Murl/murl.db %{INSTALL_DIR}/%{CUR_VERSION}/Murl
cp -r %{OUT_DEFAULT_DIR}/extensions %{INSTALL_DIR}/%{CUR_VERSION}
cp -r %{OUT_DEFAULT_DIR}/locales/en-GB.pak %{INSTALL_DIR}/%{CUR_VERSION}/locales
cp -r %{OUT_DEFAULT_DIR}/locales/en-US.pak %{INSTALL_DIR}/%{CUR_VERSION}/locales
cp -r %{OUT_DEFAULT_DIR}/locales/zh-CN.pak %{INSTALL_DIR}/%{CUR_VERSION}/locales
cp -r %{OUT_DEFAULT_DIR}/locales/zh-TW.pak %{INSTALL_DIR}/%{CUR_VERSION}/locales
cp %{OUT_DEFAULT_DIR}/nfs-browser-launcher %{INSTALL_DIR}/new_nfs-browser
cd %{buildroot}/usr/bin && ln -s /opt/nfs-browser/nfs-browser nfs-browser
rm %{INSTALL_DIR}/new_nfs-browser

%clean

rm -rf %{buildroot}

%files

%defattr(-,root,root,-)

/usr/share/applications/*

/usr/bin/*

/opt/*

%post
chmod -R o+rw /opt/nfs-browser
# Add icons to the system icons
XDG_ICON_RESOURCE="`which xdg-icon-resource 2> /dev/null || true`"
if [ ! -x "$XDG_ICON_RESOURCE" ]; then
  echo "Error: Could not find xdg-icon-resource" >&2
  exit 1
fi
for icon in "/opt/nfs-browser/icons/product_logo_"*.png; do
  size="${icon##*/product_logo_}"
  "$XDG_ICON_RESOURCE" install --size "${size%.png}" "$icon" "nfs-browser"
done

%preun


%changelog
* Wed Jul 22 2020 wqx<qixu@cpu-os.ac.cn>
- bug514 安装Flash插件时，root用户安装Flash成功，切换到普通用户需要再次安装且提示Flash安装失败
- bug516 在线播放视频和音频没有声音，但是在系统设置-声音试听中能够听到声音

* Tue Jul 13 2020 wqx<qixu@cpu-os.ac.cn> 2.4.39
- add requires

* Tue May 19 2020 wqx<qixu@cpu-os.ac.cn> 2.4.39
-
