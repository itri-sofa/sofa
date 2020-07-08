Summary:   ITRI ICL All Flash Array Storage CI Autotest scripts
Name:      SOFACI
Version:   1.0.00
Release:   1
License:   ITRI
Group:     ICL-0F000
Source:    SOFACI-1.0.00.tar.gz
Url:       http://www.itri.org.tw
Packager:  Lego
BuildRoot: %{_tmppath}/%{name}-%{version}-root

### Pter add for centos 8.1
%global debug_package %{nil} 

%description
This package will instal SOFA CI to /usr/sofa/bin/testing/ci_test

%prep
%setup -q

%build

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}/usr/local/bin
install -m 755 SOFACI_Release.tar.gz %{buildroot}/usr/local/bin/SOFACI_Release.tar.gz

%files
/usr/local/bin/SOFACI_Release.tar.gz

%post
rm -fr /usr/sofa/bin/testing/ci_test
mkdir -p /usr/sofa/bin/testing
tar -xvzf /usr/local/bin/SOFACI_Release.tar.gz -C /usr/sofa/bin/testing/
mv -f /usr/sofa/bin/testing/SOFACI_Release/ /usr/sofa/bin/testing/ci_test
chmod -R 755 /usr/sofa/bin/testing/ci_test

%changelog
* Tue Nov 02 2010 LegoLin(Lego)
- build the program
