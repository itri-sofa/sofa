Summary:   ITRI ICL All Flash Array Storage Monitor system
Name:      SOFAMon
Version:   1.0.00
Release:   1
License:   ITRI
Group:     ICL-0F000
Source:    SOFAMon-1.0.00.tar.gz
Url:       http://www.itri.org.tw
Packager:  Lego
BuildRoot: %{_tmppath}/%{name}-%{version}-root

### Pter add for centos 8.1
%global debug_package %{nil} 

%description
This package will instal SOFA Mon to /usr/sofaMon/

%prep
%setup -q

%build

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}/usr/local/bin
install -m 755 SOFAMon_Release.tar.gz %{buildroot}/usr/local/bin/SOFAMon_Release.tar.gz

%files
/usr/local/bin/SOFAMon_Release.tar.gz

%post
rm -fr /usr/sofaMon/
#mkdir -p /usr/sofaMon/
tar -xvzf /usr/local/bin/SOFAMon_Release.tar.gz -C /usr/
mv -f /usr/SOFAMon_Release/ /usr/sofaMon/
chmod -R 755 /usr/sofaMon

%changelog
* Tue Nov 02 2010 LegoLin(Lego)
- build the program
