# SOFA Test Script

## Installation Guide
* Compile and build test script RPM package (SOFACI-1.0.00.x86_64.rpm)
    ```
    $ cd  $GitClonePath/sofa
    $ sh ./gen_sofaCItest_package.sh
    ```

* Install test script
    ```
    $ cd packages
    $ sh ./undeploy_sofaCI.sh
    $ sh ./deploy_sofaCI.sh
    ```

* Run test script
    ```
    $ sh /usr/sofa/bin/testing/ci_test/citest_runall.sh &
    ```

* Check testing result
    ```
    $ tail -f /usr/sofa/bin/testing/ci_test/log/citest_runall.log
    ```
## Test Script Detail

* File system operation
    
    The scripts are put under the folder "file_testing".

    * ftest_formatfs.scp \
        Purpose: test ext2 file system format \
        Input parameters: <1st> target raw device \
        &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
        <2nd> number of test rounds

    * ftest_multidir.scp \
        Purpose: test folder creation  \
        Input parameters: 
        <1st> target raw device \
        &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
        <2nd> number of folders

    * ftest_randrw_gc.scp \
        Purpose: test file random RW in different folders \
        Input parameters: 
        <1st> target raw device \
        &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
        <2nd> number of folders

    * ftest_randsizefileRW.scp \
        Purpose: test RW on files of varying size, and test file-system unmount and re-mount \
        Input parameters:
        <1st> target raw device \
        &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
        <2nd> number of test rounds

    * ftest_r_w.scp \
        Purpose: test RW on files of specified size \
        Input parameters: 
        <1st> target raw device \
        &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
        <2nd> file size

    * iozone_rw.scp \
        Purpose: test SOFA with iozone \
        Input parameters: 
        <1st> target raw device

* Raw device operation

    The scripts are put under the folder rw_testing.
    
    * onlysw4times.scp \
        Purpose: page-based sequential write for 4 rounds \
        Input parameters: 
        <1st> target raw device \
        &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
        <2nd> total number of page writes in each round

    * scsw4times.scp \
        Purpose: sector-based sequential RW for 4 rounds \
        Input parameters:
        <1st> target raw device \
        &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
        <2nd> total number of page writes in each round \
        &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
        <3th> 4KB or 8KB (for SSD page size)

    * sw4times.scp \
        Purpose: page-based sequential RW for 4 rounds \
        Input parameters:
        <1st> target raw device \
        &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
        <2nd> total number of page writes in each round \
        &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
        <3th> 4KB or 8KB (for SSD page size)
            
    * randw_page_7.scp \
        Purpose: page-based random RW, where the length of each write is 7 pages \
        Input parameters: 
        <1st> target raw device \
        &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
        <2nd> total number of page writes

    * randw_page_32.scp \
        Purpose: page-based random RW, where the length of each write is 32 pages \
        Input parameters: 
        <1st> target raw device \
        &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
        <2nd> total number of page writes

    * randw_sector.scp \
        Purpose: sector-based random RW, where the length of each write is 32 sectors \
        Input parameters:
        <1st> target raw device \
        &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
        <2nd> total number of page writes \
        &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
        <3th> 4KB or 8KB (for SSD page size)

    * randw_Ten_times.scp \
        Purpose: page-based random RW, where the total number of page writes is 10 times of the second parameter \
        Input parameters:
        <1st> target raw device \
        &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
        <2nd> number of page writes

    * rw_perf.scp \
        Purpose: random RW performance testing \
        Input parameters:
        <1st> target raw device

            
* FIO testing

   The scrips are put under the folder fio_testing.

    * fio_test_fs.scp \
        Purpose: use fio to test file RW on varying file system type \
        Input parameters: 
        <1st> target raw device \
        &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
        <2nd> ext2, ext3, ext4, or xfs (target file system type)

    * fio_test_raw.scp \
        Purpose: use fio to test raw device RW on multiple partitions \
        Input parameters: 
        <1st> target raw device \
        &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
        <2nd> number of partitions

* RAID5 testing

    The scripts are put under the folder redundancy_testing.
    
    * rddcy_read.scp \
        Purpose: test read is not affected if one disk fails \
        Input parameters: 
        <1st> target raw device \
        &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
        <2nd> total number of page reads

    * rddcy_read_hotswap.scp \
        Purpose: test read is not affected if one disk fails and system starts the disk recovery by the spare disk \
        Input parameters: 
        <1st> target raw device \
        &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
        <2nd> total number of page reads

    * rddcy_write.scp \
        Purpose: test fail disk recovery will start automatically when a new disk is plugged in, and test the recovery correctness \
        Input parameters:
        <1st> target raw device \
        &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
        <2nd> total number of page writes before the disk failure

    * rddcy_write_hotswap.scp \
        Purpose: test fail disk recovery will start automatically by the spare disk, and test the recovery correctness \
        Input parameters:
        <1st> target raw device \
        &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
        <2nd> total number of page writes before the disk failure

    * rddcy_recov.scp \
        Purpose: test un-finished fail disk recovery will continue the work after SOFA restarts \
        Input parameters:
        <1st> target raw device \
        &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
        <2nd> total number of page writes before the disk failure


*  SOFA restart testing

    The scripts are put under the folder recovery_testing.
    
    * poweroff_page.scp \
        Purpose: test page-based random RW after SOFA restarts \
        Input parameters: 
        <1st> target raw device \
        &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
        <2nd> the device range in the unit of pages to perform RW

    * poweroff_sector.scp \
        Purpose: test sector-based random RW after SOFA restarts \
        Input parameters: 
        <1st> target raw device \
        &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
        <2nd> the device range in the unit of pages to perform RW
