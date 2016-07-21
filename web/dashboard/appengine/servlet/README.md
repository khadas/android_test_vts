# To run locally

Make sure to update the gcloud directory in pom.xml
Add the following to the pom.xml within the 'com.google.appengine' plugin tag :
  <configuration>
    <gcloud_directory>PATH/TO/GCLOUD_DIRECTORY</gcloud_directory>
  </configuration>

$ cd web/dashboard/appengine/servlet
$ mvn clean gcloud:run

# To deploy to GAE
$ mvn clean gcloud:deploy

visit https://android-vts-internal.googleplex.com
