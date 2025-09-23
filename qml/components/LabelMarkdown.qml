import QtQuick 2.0
import Sailfish.Silica 1.0

import ".."

Label {
    property bool fetching: false
    property var  url

    textFormat: Text.RichText
    Component.onCompleted: fetch()
    onUrlChanged: fetch()

    function fetch() {
        if (!url) {
            text = "";
            return;
        }

        // detect anti-bot measures in response:
        const anubisRE = new RegExp("<script[^>]+id=['\"]anubis_(version|challenge)['\"]", "mi");
        // CF is hard to detect, but usually has the attribution tag at the bottom
        const cflareRE = new RegExp("href=['\"]https://www.cloudflare.com/5xx-error-landing", "mi");
        // At least Anubis dislikes the Mozilla part of most UAs:
        const userAgent: 'SailfishOS Chum GUI/' + Qt.application.version + ' https://github.com/sailfishos-chum'

        fetching = true;
        //% "Loading..."
        text = qsTrId("chum-loading-text");
        var xhr = new XMLHttpRequest;
        xhr.setRequestHeader('User-Agent', userAgent);
        xhr.open("GET", url);
        xhr.onreadystatechange = function() {
            if (xhr.readyState === XMLHttpRequest.DONE) {
                if ((xhr.responseText === "")
                     // detect anti-bot measures and fail
                     || anubisRE.test(xhr.responseText)
                     || cflareRE.test(xhr.responseText)
                   )
                { text = "" }
                else { text = MarkdownParser.parse(xhr.responseText); }
            }
        }
        xhr.send()
    }
}
