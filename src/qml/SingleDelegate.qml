
/* KLook
 * Copyright (c) 2011-2012 ROSA  <support@rosalab.ru>
 * Authors: Julia Mineeva, Evgeniy Auzhin, Sergey Borovkov.
 * License: GPLv3
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as
 *   published by the Free Software Foundation; either version 3,
 *   or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

import QtQuick 1.1
import Widgets 1.0

Item {
    id: listItem
    width: photosListView.width; height: photosListView.height

    property int iconHeightMargin: ( height - panel.height ) / 10 // icon height is 4/5 of item height minus panel height
    property int leftItemMargin: 20    // before icon
    property int rightItemMargin: 10   // after text info
    property int middleItemMargin: 20  // middle icon and text info

    // getting of icon width for audio, mime, folder delegate.
    // width must be less 500.
    function getIconWidth( iconWidth, iconHeight, textWidth )
    {
        var w = listItem.width - textWidth - ( leftItemMargin + rightItemMargin + middleItemMargin )
        var h = listItem.height - iconHeightMargin * 2 - panel.height
        if ( w > h ) {
            w = h
        }

        if ( w > 500 ) {
            w = 500
        }

        return w
    }

    ImageDelegate {
        id: imgDelegate
    }

    VideoDelegate {
        id: videoDelegate
    }

    AudioDelegate {
        id: audioDelegate
    }

    TextDelegate {
        id: txtDelegate
    }

    FolderDelegate {
        id: folderDelegate
    }

    MimeDelegate {
        id: mimeDelegate
    }

    OkularDelegate {
        id: okularDelegate
    }

    ProgressDelegate {
        id: progressDelegate
    }

    // function for getting delegate of loader element
    function bestDelegate(t) {
        console.log("Download in progress" + downloadInProgress)
        // for some reason '===' does not work here
        if(t == File.Undefined || downloadInProgress) {
            return progressDelegate
        }
        if (t == File.Image) {
            return imgDelegate;
        }
        else if (t == File.Video) {
            return videoDelegate;
        }
        else if (t == File.Txt) {
            return txtDelegate;
        }
        else if (t === File.Directory) {
            return folderDelegate;
        }
        else if (t == File.Audio) {
            return audioDelegate;
        }
        else if (t == File.OkularFile) {
            return okularDelegate;
        }
        else {
            return mimeDelegate;
        }
        console.debug("Should never be here")
    }

    Loader
    {
        id: componentLoader
        anchors.fill: parent;
        sourceComponent: bestDelegate(type)
    }
}
