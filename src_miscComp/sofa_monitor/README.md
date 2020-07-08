# SOFA Web UI

* Install necessary packages
    ```
    $ yum -y install ant
    ```
* Install Glassfish server
    ```
    $ wget http://download.oracle.com/glassfish/5.0.1/nightly/latest-glassfish.zip
    $ unzip latest-glassfish.zip -d /usr
    $ /usr/glassfish5/glassfish/bin/asadmin start-domain
    $ firewall-cmd --zone=public --permanent --add-port 8080/tcp
    $ firewall-cmd --reload
    ```
* Compile and build RPM package (SOFAMon-1.0.00.x86_64.rpm)
    ```
    $ cd  $GitClonePath/sofa
    $ sh ./gen_sofaMon_package.sh
    ```
* Install SOFA Web UI
    ```
    $ cd packages
    $ sh ./undeploy_sofaMon.sh
    $ sh ./deploy_sofaMon.sh
    $ /usr/glassfish5/glassfish/bin/asadmin deploy /usr/sofaMon/SOFAUI.war
    $ /usr/glassfish5/glassfish/bin/asadmin deploy /usr/sofaMon/SOFAMonitor.war
    ```

* Connect to SOFA Web UI \
    The URL is http://ServerIP:8080/SOFAUI
