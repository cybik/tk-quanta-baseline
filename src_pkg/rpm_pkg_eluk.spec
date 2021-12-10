%define module module-name

#
# spec file for package tuxedo-keyboard
#
# Copyright (c) 2019 SUSE LINUX GmbH, Nuernberg, Germany.
#
# All modifications and additions to the file contributed by third parties
# remain the property of their copyright owners, unless otherwise agreed
# upon. The license for this file, and modifications and additions to the
# file, is the same license as for the pristine package itself (unless the
# license for the pristine package is not an Open Source License, in which
# case the license is the MIT License). An "Open Source License" is a
# license that conforms to the Open Source Definition (Version 1.9)
# published by the Open Source Initiative.

# Please submit bugfixes or comments via http://bugs.opensuse.org/
#


Summary:        Kernel module Set for the Eluktronics Prometheus XVI WMI LED Controls
Name:           %{module}
Version:        0.1.0
Release:        x
License:        GPLv3+
Group:          Hardware/Other
BuildArch:      noarch
Url:            https://github.com/cybik/tk-quanta-elukxvi
Source:         %{module}-%{version}.tar.bz2
Provides:       eluk-pxvi-wmi = %{version}-%{release}
Requires:       dkms >= 1.95
BuildRoot:      %{_tmppath}
Packager:       Renaud Lepage <root@cybikbase.com>

%description
Unofficial WMI control driver for the Eluktronics Prometheus XVI,
meant for use with the DKMS framework.

%prep
%setup -n %{module}-%{version} -q

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}/usr/src/%{module}-%{version}/
cp dkms.eluk.conf %{buildroot}/usr/src/%{module}-%{version}
cp Makefile %{buildroot}/usr/src/%{module}-%{version}/Makefile
cp -R src %{buildroot}/usr/src/%{module}-%{version}
mkdir -p %{buildroot}/etc/udev/rules.d/
cp 90_eluk_wmi.rules %{buildroot}/etc/udev/rules.d/
mkdir -p %{buildroot}/usr/share/
mkdir -p %{buildroot}/usr/share/%{module}/
cp postinst %{buildroot}/usr/share/%{module}

%clean
rm -rf %{buildroot}

%files
%defattr(0644,root,root,0755)
%attr(0755,root,root) /usr/src/%{module}-%{version}/
%attr(0644,root,root) /usr/src/%{module}-%{version}/*
%attr(0755,root,root) /usr/src/%{module}-%{version}/src/
%attr(0644,root,root) /usr/src/%{module}-%{version}/src/*
%attr(0755,root,root) /usr/share/%{module}/
%attr(0755,root,root) /usr/share/%{module}/postinst
%license LICENSE

%post
for POSTINST in /usr/lib/dkms/common.postinst /usr/share/%{module}/postinst; do
    if [ -f $POSTINST ]; then
        $POSTINST %{module} %{version} /usr/share/%{module}
        RET=$?

        # Attempt to (re-)load module immediately, fail silently if not possible at this stage
        echo "(Re)load modules if possible"

        rmmod eluk-pxvi-led-wmi > /dev/null 2>&1 || true
        
        modprobe eluk-pxvi-led-wmi > /dev/null 2>&1 || true

        exit $RET
    fi
    echo "WARNING: $POSTINST does not exist."
done

echo -e "ERROR: DKMS version is too old and %{module} was not"
echo -e "built with legacy DKMS support."
echo -e "You must either rebuild %{module} with legacy postinst"
echo -e "support or upgrade DKMS to a more current version."
exit 1


%preun
echo -e
echo -e "Uninstall of %{module} module (version %{version}-%{release}) beginning:"
dkms remove -m %{module} -v %{version} --all --rpm_safe_upgrade
if [ $1 != 1 ];then
    /usr/sbin/rmmod %{module} > /dev/null 2>&1 || true
fi
exit 0


%changelog
* Mon Dec 6 2021 R Lepage <root@cybikbase.com> 3.0.9-1
- Initial release
