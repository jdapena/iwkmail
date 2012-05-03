/***************************************************************************
 * common.js : Common tools for UI, including globalStatus handling
 ***************************************************************************/

/*
 * Authors:
 *  Jose Dapena Paz <jdapena@igalia.com>
 *
 * Copyright (c) 2012, Igalia, S.L.
 * All rights reserved.
 *
 * See license.js
 */

var globalStatus = {
    currentAccount: null,
    currentFolder: null,
    folders: [],
    newestUid: null,
    oldestUid: null,
    requests: { }
};

function showError (e)
{
    $("#error-dialog #error-message").text(e);
    $.mobile.changePage("#error-dialog");
}

function getCurrentFolder()
{
    for (i in globalStatus.folders) {
	account = globalStatus.folders[i];
	if (account.accountId == globalStatus.currentAccount) {
	    return account.folders[globalStatus.currentFolder];
	}
    }
}

function updateFoldersDisplayNames ()
{
    for (i in globalStatus.folders) {
	account = globalStatus.folders[i];
	for (folderFullName in account.folders) {
	    folder = account.folders[folderFullName];
	    displayName = folder.displayName;
	    parent = folder;
	    while ('parentFullName' in parent) {
		parent = account.folders[parent.parentFullName];
		displayName = parent.displayName + "/" + displayName;
	    }
	    folder.fullDisplayName = displayName;
	}
    }
}

function globalSetFolders (foldersData)
{
    globalStatus.folders = foldersData;
    updateFoldersDisplayNames();
}

function iwkRequest (method, inData)
{
    return $.ajax({
	type: "GET",
	crossDomain: true,
	isLocal: true,
	dataType: "jsonp",
	url: "iwk:"+method,
	data: inData
    });
}