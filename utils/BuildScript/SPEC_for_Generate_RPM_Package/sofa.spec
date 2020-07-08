Summary:   ITRI ICL All Flash Array Storage system
Name:      SOFA
Version:   1.0.00
Release:   1
License:   ITRI
Group:     ICL-0F000
Source:    SOFA-1.0.00.tar.gz
Url:       http://www.itri.org.tw
Packager:  Lego
BuildRoot: %{_tmppath}/%{name}-%{version}-root

### Pter add for centos 8.1
%global debug_package %{nil} 

%description
This package will instal sofa to /usr/sofa/

%prep
%setup -q

%build

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}/usr/local/bin
install -m 755 SOFA_Release.tar.gz %{buildroot}/usr/local/bin/SOFA_Release.tar.gz

%files
/usr/local/bin/SOFA_Release.tar.gz

%post
rm -fr /usr/sofa/
#mkdir -p /usr/sofa/
tar -xvzf /usr/local/bin/SOFA_Release.tar.gz -C /usr/
mv -f /usr/SOFA_Release/ /usr/sofa/
chmod -R 755 /usr/sofa
chmod -R 777 /usr/sofa/bin/*.sh
chmod -R 777 /usr/sofa/bin/tool/disk_tool/*.sh
chmod -R 777 /usr/sofa/bin/tool/disk_tool/*.scp


%changelog
* Tue Nov 02 2010 LegoLin(Lego)
- build the program
