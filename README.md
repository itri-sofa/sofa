# What is SOFA?

SOFA (Software Orchestrated Flash Array) is a software-defined storage system to manage all flash array, and features to provide high data protection while also achieving high random access. The system provides a block device for users to read and write data, and data will be stored distributedly across all SSDs by RAID5 protection.

The advantages of SOFA includes
* only commodity hardware is needed
* data protection by RAID5
* automatic and online failed RAID recovery
* higher random data access and longer lifetime of SSD as compared to the standard software RAID5 by the special design of converting random write to sequential write
* global wear leveling across SSDs 
* web UI for system health monitoring


# Installation Guide
## Requirement

* Minimum specification

    CPU: (1) PC: Intel i5 CPU with 4 virtual CPUs and 3.30 GHz each, (2) Server: Intel CPU with 10 virtual CPUs and 2.5 GHz each  \
    Memory: formula-based,  \
        &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;size = the smallest size of SSDs (GB) \
        &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;num = total number of SSDs \
        &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;R = 1 - lfsm_seu_reserved/1000 (Note: lfsm_seu_reserved is set in [configuration file](conf/sofa_config.xml)) \
        &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;Total memory (MB) = (size * num * (2R + 0.2) + 40) * 1.2 \
    Motherboard: No special requirement \
    HBA card:  Support the trim command and no hardware RAID support \
    SSD: At least 3 SSDs with at least 250G each. SSDs must support the trim command. \
    OS: CentOS 8.1 

* Verified specification

    Server CPU: (1) Intel Xeon E5-2687W v3 (3.50 GHz), (2) Intel Xeon E5-2667 v3 (3.20 GHz), (3) Intel Xeon E5-2643 v2 (3.50GHz) \
    Memory: 64G \
    Motherboard: Supermicro X10DRU-i+ version 1.02A \
    HBA card: (1) LSI Logic / Symbios Logic SAS3008 PCI-Express Fusion-MPT SAS-3, (2) LSI SAS2008 PCI-Express Fusion-MPT SAS-2, (3) LSI SAS2308 PCI-Express Fusion-MPT SAS-2 \
    SSD: SanDisk SDSSDH3, SanDisk Ultra II 00RL \
    OS:  [CentOS-8.1.1911-x86_64-dvd1.iso (CentOS 8.1.1911)](http://isoredirect.centos.org/centos/8/isos/x86_64/CentOS-8.1.1911-x86_64-dvd1.iso) 

* Guide to install HW drivers in CentOS 8

    Please refer to the [website](https://access.redhat.com/documentation/en-us/red_hat_enterprise_linux/8/html/considerations_in_adopting_rhel_8/hardware-enablement_considerations-in-adopting-rhel-8#removed-adapters_hardware-enablement) to check the devices not supported by CentOS 8.
    You can also refer to the [website](https://elrepoproject.blogspot.com/2019/08/rhel-80-and-support-for-removed-adapters.html) or [tutorial vedio](https://www.youtube.com/watch?v=4fOAuXiynYM&feature=youtu.be) to install the drivers of the unsupported hardware in CentOS 8.

## Build and Install SOFA

* Install necessary packages 
    ```
    $ uname -r
    4.18.0-147.el8.x86_64
    $ yum -y install gcc
    $ yum -y install gcc-c++
    $ yum -y install epel-release
    $ yum install kernel-devel-`uname -r`
    $ dnf --enablerepo=PowerTools install json-c-devel
    $ yum -y install rpm-build
    $ yum -y install libaio-devel
    $ yum -y install make
    $ yum -y install elfutils-libelf-devel
    ```
    If you fail on the step "yum install kernel-devel-\`uname -r\`", you may need to first do "yum update" and reboot server and then retry this step again. 

* Get source codes
    ```
    $ git clone https://github.com/itri-sofa/sofa.git
    ```

* Compile and build RPM package (SOFA-1.0.00.x86_64.rpm)\
    $GitClonePath is the directory path where you clone SOFA project.
    ```
    $ cd $GitClonePath/sofa
    $ sh ./gen_sofa_package.sh
    ./gen_sofa_package.sh platform=linux verion=1.0.00 release=1 tmp_dir=$GitClonePath/sofa/SOFA_Release/
    ./gen_sofa_package.sh start to generate sofa release packages
    .......................................
    .......................................
    + exit 0
    RPM Preparation Done!
    Generate sofa packages success
    ```

* Install SOFA
    ```
    $ cd packages
    $ sh ./undeploy_sofa.sh
    $ sh ./deploy_sofa.sh 
    ./deploy_sofa.sh: deply SOFA version=1.0.00 release=1
    Preparing...                          ################################# [100%]
    Updating / installing...
    .......................................
    .......................................
    SOFA_Release/lfsmdr.ko
    SOFA_Release/rfsioctl
    ./deploy_sofa.sh: deploy SOFA done
    ```

* Check SOFA installation result
    ```
    $  ls -l /usr/sofa
    drwxr-xr-x. 3 root root      194 Apr  8 14:10 bin
    drwxr-xr-x. 2 root root       29 Apr  8 14:12 config
    -rwxr-xr-x. 1 root root 19741416 Apr  8 14:10 lfsmdr.ko
    -rwxr-xr-x. 1 root root    15960 Apr  8 14:10 rfsioctl
    -rwxr-xr-x. 1 root root  1769752 Apr  8 14:10 sofa.ko
    -rwxrwxrwx. 1 root root    56616 Apr  8 14:10 sofa_daemon
    ```

## Configure SOFA

* Configuration file \
The configuration file is named [sofa_config.xml](conf/sofa_config.xml), and it is placed under folder /usr/data/sofa/config after installation.

- Key settings
    - lfsm_reinstall   : Whether SOFA needs to clean previous data in the next startup \
          - value      : 0: keep previous data; 1: clean previous data

    - lfsm_cn_ssds     : The SSDs for SOFA   \
          - value      : Number of SSDs, which should be less than or equal to 64. Please make sure that lfsm_cn_ssds >= cn_ssds_per_hpeu * lfsm_cn_prgroup. The number of spare disks will be the value of lfsm_cn_ssds - cn_ssds_per_hpeu * lfsm_cn_prgroup.  \
          - settings   : List of SSD device names separated by comma. The number of SSD device names listed here must match the one in "-value" tag. Please NOT use the system disk. Please also be noted that these SSDs will be erased during SOFA startup. \
          [Must be configured] 
          
    - cn_ssds_per_hpeu : How many SSDs in a RAID5 group  \
          - value      : Number of SSDs in a RAID5 group, which should be within the range [3, 15]\
          [Must be configured]
                         
    - lfsm_cn_pgroup   : How many RAID5 groups  \
          - value      : Number of RAID5 groups  \
          [Must be configrued]

    - lfsm_seu_reserved: The permille of total space reserved for garbage collection \
          - value      : The permille of total space 

    - lfsm_gc_chk_intvl: The interval of garbage collecion checking \
          - value      : The interval expressed in milliseconds 

    - lfsm_gc_on_lower_ratio: The threshold conditioned on free space to trigger garbage collection \
          - value      : The permille of free space 

    - lfsm_gc_off_upper_ratio: The threshold conditioned on free space to stop garbage collection \
          - value      : The permille of free space 

    - lfsm_io_thread   : The amount and vcores for SOFA IO threads \
          - value      : Number of SOFA IO threads \
          - settings   : List of vcore IDs seperated by comma for specific assignments or "-1" for system-automatic assignment. The number of vcores listed here must match the one in "-value" tag. 

    - lfsm_bh_thread   : The amount and vcores for SOFA bottom-half threads \
          - value      : Number of SOFA bottom-half threads \
          - settings   : List of vcore IDs seperated by comma for specific assignments or "-1" for system-automatic assignment. The number of vcores listed here must match the one in "-value" tag. 

          
* Example guide for the configurations

    This example shows you how to set up the configurations that must be changed based on your system environment. We first check out the SSDs in the machine.
    ```
    $ lsblk
    NAME   MAJ:MIN RM   SIZE RO TYPE MOUNTPOINT
    sda      8:0    0 931.5G  0 disk 
    |-sda1   8:1    0   500M  0 part /boot
    |-sda2   8:2    0  31.4G  0 part [SWAP]
    |-sda3   8:3    0    50G  0 part /
    |-sda4   8:4    0     1K  0 part 
     -sda5   8:5    0 849.6G  0 part /home
    sdb      8:16   0 447.1G  0 disk 
    sdc      8:32   0 447.1G  0 disk 
    sdd      8:48   0 447.1G  0 disk 
    sde      8:64   0 447.1G  0 disk 
    sdf      8:80   0 447.1G  0 disk 
    sdg      8:96   0 447.1G  0 disk 
    sdh      8:112  0 447.1G  0 disk 
    ```
          
    In this example, there are 7 available SSDs from sdb to sdh, except for sda used for the operation system. If we decide to have 2 RAID5 groups with 3 SSDs each and have 1 spare disk, the configurations will be set as shown below.
    ```
    <property>
        <name>lfsm_cn_ssds</name>
        <value>7</value>
        <setting>sdb,sdc,sdd,sde,sdf,sdg,sdh</setting>
    </property>
    <property>
        <name>cn_ssds_per_hpeu</name>
        <value>3</value>
    </property>
    <property>
        <name>lfsm_cn_pgroup</name>
        <value>2</value>
    </property> 
    ```

## Run and Stop SOFA

* Run SOFA
    ```
    $ sh /usr/sofa/bin/start-sofa.sh
    ```

* Check if SOFA services are available

    Check kernel log with specific message
    ```
    $ dmesg | grep "main INFO initial all sofa components done"
    [239305.799428] [SOFA] main INFO initial all sofa components done
    ```
    
    Check if /dev/lfsm exists
    ```
    $ lsblk
    .......................................
    sdh     65:96   0 447.1G  0 disk 
    lfsm   252:0    0   1.3T  0 disk
    ```

    Now, SOFA virtual device /dev/lfsm is ready to use. You can perform RW on this virtual device just like a normal physical raw device.


* Stop SOFA
    ```
    $ sh /usr/sofa/bin/stop-sofa.sh
    ```

## Uninstall SOFA
    
* Before uninstalling SOFA, please make sure SOFA is stopped.
    ```
    $ sh $GitClonePath/sofa/packages/undeploy_sofa.sh
    ```

# Installation Guide for SOFA Web UI
There is a Web UI for users to monitor SOFA health and do some configuration changes. To build and run this Web UI, please see [src_miscComp/sofa_monitor/README.md](src_miscComp/sofa_monitor/README.md).

# Community
* SOFA main channel \
To get help or provide any feedback, the main channel is the [SOFA mailing list](https://groups.google.com/forum/#!forum/itri-sofa). \
For bug reports, please just use [GitHub](https://github.com/itri-sofa/sofa).

* Coding style \
Contributions are welcomed. SOFA coding style follows [linux kernel coding style](https://www.kernel.org/doc/html/v4.10/process/coding-style.html) except indentations. For indentations SOFA coding style replaces the tab with 4 blank characters. Please refer to linux kernel coding style before the contribution and read the [contributing file](contributing.txt) to know how to contribute your codes.

* Test script \
The guide for SOFA test scripts is shown in [utils/ci_autotest/README.md](utils/ci_autotest/README.md).


# SOFA Commercial Version

The advanced features includes 
* 1M IOPS for 4KB random RW over iSCSI - When the users access the block device over iSCSI from remote machines, SOFA is still able to reach 1M IOPS.
* RAID6 protection - SOFA can tolerate two disk failures for the whole system. 
* Volume manager - SOFA supports thin-provisioned volumes and snapshots.
* Scale-up - The block device provided by SOFA can be scaled up on-the-fly by using the spare disks in the machine.
* Write limit - Users can control the write amount on the SSDs to prevent disk failures. 
* High availability (HA) - Two SOFA running in active-active mode in two servers can achieve no service downtime.

## Contact Us for Commercial Version

Organization: Industrial Technology Research Institute \
Contact person: Vicki Chu \
Email: vicki.chu@itri.org.tw \
