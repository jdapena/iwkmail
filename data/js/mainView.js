/***************************************************************************
 * mainView.js : UI control implementation for main-view.html
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

function fillAccountsList (accounts)
{
    clearAccountsList ();

    for (i in accounts) {
	dumpAccountInAccountsList (accounts[i], "#page-accounts #accounts-list");
    }
    dumpAddAccountInAccountsList ("#page-accounts #accounts-list");

    if ($(parent).hasClass("ui-listview")) {
	$(parent).listview("refresh");
    }
    if ($("#accounts-list").hasClass("ui-listview"))
	$("#accounts-list").listview('refresh');
}

function fillAccountsListCounts ()
{
    if ($("accounts-list").hasClass("ui-listview"))
	$("#accounts-list").listview('refresh');

    for (i in globalStatus.folders) {
	account = globalStatus.folders[i];
	accountId = account.accountId;
	inbox = account.folders["INBOX"];
	if (!inbox)
	    inbox = account.folders["inbox"];
	accountItemId = "account-item-"+accountId;
	if (inbox && inbox.unreadCount && inbox.unreadCount > 0) {
	    $("#page-accounts #account-item-"+accountId+" .account-count").text(inbox.unreadCount);
	    $("#page-accounts #account-item-"+accountId+" .account-count").show();
	} else {
	    $("#page-accounts #account-item-"+accountId+" .account-count").hide();
	}
    }
    if ($("#accounts-list").hasClass("ui-listview"))
	$("#accounts-list").listview('refresh');
}

function fillFoldersList(accountId)
{
    for (i in globalStatus.folders) {
	account = globalStatus.folders[i];
	if (account.accountId == accountId) {
	    $("#page-folders-title").text(account.accountName + " folders");

	    $("#page-folders #folders-list").html("");
	    for (fullName in account.folders) {

		folder = account.folders[fullName];
		dumpFolderInFolderList (accountId, fullName, folder,
					"#page-folders #folders-list");
		
		if (fullName == "INBOX") {
		    accountItemId = "account-item-"+accountId;
		    $("#page-accounts #"+accountItemId+" .countSpan:first").text(folder.unreadCount);
		    if (folder.unreadCount > 0)
			$("#page-accounts #"+accountItemId+" .countSpan:first").show();
		    else
			$("#page-accounts #"+accountItemId+" .countSpan:first").hide();
		}
	    }
	    if ($("#folders-list").hasClass("ui-listview"))
		$("#folders-list").listview('refresh');

	    break;
	}
    }
}

function fillMessageViewHeader (message)
{
    dumpMessageAsViewHeader (message, "#page-message");
}

function fillMessageDetails (message)
{
    dumpMessageAsDetails (message, "#message-details-list");
}

function fillMessageViewBody (message)
{
    globalStatus.messageStructure = message;
    dumpDataWrapper (message, "#page-message #iframe-container");
}

function clearMessageViewBody ()
{
    $("#page-message #iframe-container").text("");
}

function clearAccountsList ()
{
    $("#accounts-list").html("");
}

function abortShowMessages ()
{
    if ('showMessages' in globalStatus.requests) {
	globalStatus.requests["showMessages"].abort();
    }
}

function markMessageAsRead(uid)
{
    iwkRequest ("flagMessage", "Marking message as read", {
	account: globalStatus.currentAccount,
	folder: globalStatus.currentFolder,
	message: uid,
	setFlags: "seen"
    }).done(function (msg) {
	$("#message-item-"+uid).removeClass("iwk-unread-item");
	$("#message-item-"+uid).addClass("iwk-read-item");
	$("#message-view-mark-as-read").hide();
	$("#message-view-mark-as-unread").show()
    });
}

function markMessageAsUnread(uid)
{
    iwkRequest ("flagMessage", "Marking message as unread", {
	account: globalStatus.currentAccount,
	folder: globalStatus.currentFolder,
	message: uid,
	unsetFlags: "seen"
    }).done(function (msg) {
	$("#message-item-"+uid).removeClass("iwk-read-item");
	$("#message-item-"+uid).addClass("iwk-unread-item");
	$("#message-view-mark-as-unread").hide();
	$("#message-view-mark-as-read").show();
    });
}

function markAsRead ()
{
    markMessageAsRead (globalStatus.currentMessage);
}

function markAsUnread ()
{
    markMessageAsUnread (globalStatus.currentMessage);
}

function markMessageAsDeleted(uid)
{
    $(".iwk-message-item[data-iwk-message-id='"+uid+"']").hide ();
    iwkRequest ("flagMessage", "Marking message as deleted", {
	account: globalStatus.currentAccount,
	folder: globalStatus.currentFolder,
	message: uid,
	setFlags: "deleted",
    }).done(function (msg) {
    });
}

function deleteMessage ()
{
    markMessageAsDeleted (globalStatus.currentMessage);
    history.back ();
}

function hasBlockedImages()
{
    globalStatus.hasBlockedImages = true;
    $("#page-message-blocked-images-banner").show();
}

function unblockImages ()
{
    iwkRequest ("flagMessage", "Marking message as unblockImages", {
	account: globalStatus.currentAccount,
	folder: globalStatus.currentFolder,
	message: globalStatus.currentMessage,
	setFlags: "unblockImages"
    }).done(function (msg) {
	$("#page-message-blocked-images-banner").hide();
	clearMessageViewBody ();
	fillMessageViewBody (globalStatus.messageStructure);
    });
    
}

function showMessage(message)
{
    globalStatus.messageStructure = null;
    globalStatus.hasBlockedImages = false;
    globalStatus.showImages = false;
    $("#page-message-blocked-images-banner").hide();
    clearMessageViewBody ();
    fillMessageViewHeader (message);
    if ('getMessage' in globalStatus.requests) {
	globalStatus.requests['getMessage'].abort();
    }

    globalStatus.requests["getMessage"] = iwkRequest ("getMessage", "Getting message", {
	account: globalStatus.currentAccount,
	folder: globalStatus.currentFolder,
	message: message.uid
    }).done(function (msg) {
	fillMessageViewBody (msg.result);
	fillMessageDetails (msg.result);
	markMessageAsRead (message.uid);
    }).always(function(jqXHR, textStatus, errorThrown) {
	if ('getMessage' in globalStatus.requests)
	    delete globalStatus.requests["getMessage"];
    });
}

function showNextMessage()
{
    current = null;
    $("#messages-list .iwk-message-item-link").each (function (index) {
	if (current != null) {
	    globalStatus.currentMessage = this.messageUid;
	    showMessage (this.message);
	    return false;
	}
	if (this.messageUid == globalStatus.currentMessage)
	    current = this;
    });
    $("#message-view-next-button").removeClass("ui-btn-active");
    return true;
}

function showPreviousMessage ()
{
    previous = null;
    $("#messages-list .iwk-message-item-link").each (function (index) {
	if (this.messageUid == globalStatus.currentMessage) {
	    if (previous) {
		globalStatus.currentMessage = previous.messageUid;
		showMessage (previous.message);
	    }
	    return false;
	}
	previous = this;
    });
    $("#message-view-previous-button").removeClass("ui-btn-active");
    return true;
}

function showMessages(accountId, folderId, onlyNew)
{
    try {
	if ('showMessages' in globalStatus.requests) {
	    globalStatus.requests["showMessages"].abort();
	}

	$("#messages-list-get-more-list").hide();
	$("#messages-list-getting-more-list").show();
	$("#messages-refresh").hide();
	$("#messages-cancel").show();

	retrieveCount = onlyNew?0:SHOW_MESSAGES_COUNT;
	globalStatus.requests["showMessages"] = iwkRequest ("getMessages", "Fetching messages", {
	    account: accountId,
	    folder: folderId,
	    newestUid: globalStatus.newestUid,
	    oldestUid: globalStatus.oldestUid,
	    count: retrieveCount
	}).done(function (msg) {
	    if (msg.result.newMessages.length > 0) {
		globalStatus.newestUid = msg.result.newMessages[0].uid;
	    }
	    if (globalStatus.newestUid == null && msg.result.messages.length > 0) {
		globalStatus.newestUid = msg.result.messages[0].uid;
	    }
	    if (msg.result.messages.length > 0) {
		globalStatus.oldestUid = msg.result.messages[msg.result.messages.length - 1].uid;
	    }
	    msg.result.newMessages.reverse();
	    for (i in msg.result.newMessages) {
		dumpMessageInMessagesList (msg.result.newMessages[i], true, "#page-messages #messages-list");
	    }
	    for (i in msg.result.messages) {
		dumpMessageInMessagesList (msg.result.messages[i], false, "#page-messages #messages-list");
	    }
	    if ($("#messages-list").hasClass("ui-listview"))
		$("#messages-list").listview('refresh');
	}).always(function(jqXHR, textStatus, errorThrown) {
	    if ('showMessages' in globalStatus.requests)
		delete globalStatus.requests["showMessages"];
	    $("#messages-list-get-more-list").show();
	    $("#messages-list-getting-more-list").hide();
	    $("#messages-refresh").show();
	    $("#messages-cancel").hide();
	});
    } catch (e) {
	console.log(e.message);
    }
}

function fetchMoreMessages ()
{
    showMessages (globalStatus.currentAccount, globalStatus.currentFolder, false);
}

function fetchNewMessages ()
{
    if (globalStatus.currentAccount != null &&
	globalStatus.currentFolder != null)
	showMessages (globalStatus.currentAccount, globalStatus.currentFolder, true);
}

function refreshAccounts ()
{
    
    result = imAccountMgr.getAccounts ();
    result.onSuccess = function (result) {
	globalStatus.accounts = result;
	fillComposerFrom (result);
	fillAccountsList (result);
	syncFolders();
	console.log (JSON.stringify(result));
    }
}

function syncAllAccounts ()
{
    for (i in globalStatus.accounts) {
	var account = globalStatus.accounts[i];
	iwkRequest("syncAccount", "Synchronizing account "+account.id, {
	    account: account.id
	}).done(function (msg) {
	    globalSetAccountFolders (msg.result.accountId, msg.result);
	    fillAccountsListCounts ();
	    fillFoldersList(globalStatus.currentAccount);
	});
    }
}

function runSendQueues ()
{
    for (i in globalStatus.accounts) {
	var account = globalStatus.accounts[i];
	iwkRequest("runSendQueue", "Running account "+account.id+" send queue", {
	    account: account.id
	}).done(function (msg) {
	});
    }
}

function syncFolders ()
{
    iwkRequest("syncOutboxStore", "Syncing local outbox store", {
    }).complete(function (msg) {
	syncAllAccounts ();
	runSendQueues ();
    });
}

function deleteAccount ()
{
    result = imAccountMgr.deleteAccount (globalStatus.currentAccount);
    result.onSuccess = function (result) {
	globalStatus.currentFolder = null;
	globalStatus.currentMessage = null;
	refreshAccounts ();
	history.go(-2);
    }
    result.onError = function (result) {
	showError (result.message);
    }
}

$(function () {
    $("#page-message-blocked-images-banner").hide();
    refreshAccounts();
});
