import QtQuick 2.0
import Sailfish.Silica 1.0
import QtQuick.XmlListModel 2.0
import org.nemomobile.models 1.0 as NM

Page {
    id: page
    allowedOrientations: Orientation.All

    // we assume the same config for chum and chum:testing for now
    property string url: "https://build.sailfishos.org/public/source/sailfishos:chum/_meta"
    property string arch //: "armv8el"
    property string selected

    BusyIndicator {
      id: busyInd
      anchors.centerIn: parent
      running: metaModel.status === XmlListModel.Loading
      size: BusyIndicatorSize.Large
    }

    NM.FilterModel { id: repoModel
        filters: [ { 'role': 'arch', 'comparator': '==', 'value': page.arch }, ]
        sourceModel: metaModel
    }

    XmlListModel { id: metaModel
        source: page.url
        /*
           <repository name="3.4.0.24_armv7hl">
           <path project="sailfishos:3.4.0.24" repository="latest_armv7hl"/>
           <arch>armv8el</arch>
           </repository>
         */
        query: "//project/repository"
        XmlRole{ name: "repo";     query: "@name/string()" }
        XmlRole{ name: "longver";  query: 'substring-before(@name, "_")' }
        XmlRole{ name: "shortver";  query: 'substring(substring-before(@name, "_"), 1, 3)' }
        //XmlRole{ name: "prjname";  query: "path/@project/string()" }
        //XmlRole{ name: "reponame"; query: "path/@repository/string()" }
        XmlRole{ name: "arch";     query: "arch/string()" }
    }

    SilicaListView { id: view
        anchors.fill: parent
        model: repoModel
        header: PageHeader {
            //% "Repositories"
            title: qsTrId("chum-reposelect-title")
            description: page.arch
        }
        section {
            delegate: SectionHeader { text: section }
            property: "shortver"
        }
        delegate: ListItem {
          Label {
               text: longver
               x: Theme.horizontalPageMargin
               anchors.verticalCenter: parent.verticalCenter
           }
          onClicked: {
              page.selected = longver
              pageStack.pop()
          }
        }
        ViewPlaceholder { id: errorView
            //% "Could not load list."
            text: qsTrId("chum-reposelect-errorlabel")
            enabled: metaModel.status === XmlListModel.Error
                  || metaModel.status === XmlListModel.Null
        }
    }
    Component.onCompleted: {
      var r = new XMLHttpRequest()
      r.open('GET', 'file:///etc/ssu/ssu.ini');
      r.setRequestHeader('X-Requested-With', 'XMLHttpRequest');
      r.onreadystatechange = function(event) {
      if (r.readyState == XMLHttpRequest.DONE) {
          r.response.split("\n").forEach(
          function(line) {
              var val = /^arch=(.+)/.exec(line)
              if (val != null) {
                  page.arch = val[1]
                  return
              }
          })
      }}
      r.send();
    }
}
