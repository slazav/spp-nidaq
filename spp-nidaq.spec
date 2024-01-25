Name:         spp-nidaq
Version:      1.0
Release:      alt1

Summary:      SPP interface to NI DAQ devices (via NIDAQmx library)
Group:        System
URL:          https://github.com/slazav/spp-nidaq
License:      GPL

Packager:     Vladislav Zavjalov <slazav@altlinux.org>

Source:       %name-%version.tar
BuildRequires: gcc-c++

%description
SPP interface to NI DAQ devices (via NIDAQmx library)

%prep
%setup -q

%build
%make

%install
%makeinstall initdir=%buildroot%_initdir

%files
%_bindir/spp-nidaq

%changelog
