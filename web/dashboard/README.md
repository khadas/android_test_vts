# To run locally

Make sure to update the gcloud directory in pom.xml
Add the following to the /vts/web/dashboard/appengine/servlet/pom.xml
within the 'com.google.appengine' plugin tag :

  <configuration>
    <gcloud_directory>PATH/TO/GCLOUD_DIRECTORY</gcloud_directory>
  </configuration>

Also make sure that maven version is > 3.1
Type the following command to check the maven version.
mvn -v

The output should resemble this:

Apache Maven 3.3.9 (bb52d8502b132ec0a5a3f4c09453c07478323dc5; 2015-11-10T08:41:47-08:00)
Maven home: /opt/apache-maven-3.3.9
Java version: 1.8.0_45-internal, vendor: Oracle Corporation
Java home: /usr/lib/jvm/java-8-openjdk-amd64/jre
Default locale: en_US, platform encoding: UTF-8
OS name: "linux", version: "3.13.0-88-generic", arch: "amd64", family: "unix"

Steps to run:

$ cd web/dashboard/appengine/servlet
$ mvn clean gcloud:run

# To deploy to GAE
$ mvn clean gcloud:deploy

visit https://android-vts-internal.googleplex.com
