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
	globalStatus.requests["showMessages"].cancel();
	delete globalStatus.requests["showMessages"];
    }
}

function markMessageAsRead(uid)
{
    op = iwk.ServiceMgr.flagMessage(globalStatus.currentAccount,
				    globalStatus.currentFolder,
				    uid,
				    "seen",
				    "");
    op.opId = addOperation (result, "Flagging message as read");
    op.onSuccess = function (result) {
	$("#message-item-"+uid).removeClass("iwk-unread-item");
	$("#message-item-"+uid).addClass("iwk-read-item");
	$("#message-view-mark-as-read").hide();
	$("#message-view-mark-as-unread").show()
    };
    op.onFinish = function () {
	removeOperation (this.opId);
    };
}

function markMessageAsUnread(uid)
{
    op = iwk.ServiceMgr.flagMessage(globalStatus.currentAccount,
				    globalStatus.currentFolder,
				    uid,
				    "",
				    "seen");
    op.opId = addOperation (result, "Flagging message as unread");
    op.onSuccess = function (result) {
	$("#message-item-"+uid).removeClass("iwk-read-item");
	$("#message-item-"+uid).addClass("iwk-unread-item");
	$("#message-view-mark-as-unread").hide();
	$("#message-view-mark-as-read").show();
    };
    op.onFinish = function () {
	removeOperation (this.opId);
    };
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
    op = iwk.ServiceMgr.flagMessage(globalStatus.currentAccount,
				    globalStatus.currentFolder,
				    uid,
				    "deleted",
				    "");
    op.opId = addOperation (result, "Marking message as deleted");
    $(".iwk-message-item[data-iwk-message-id='"+uid+"']").hide ();
    op.onFinish = function () {
	removeOperation (this.opId);
    }
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
    op = iwk.ServiceMgr.flagMessage(globalStatus.currentAccount,
				    globalStatus.currentFolder,
				    globalStatus.currentMessage,
				    "unblockImages",
				    "");
    op.opId = addOperation (op, "Marking unblockImages flag on message");
    op.onSuccess = function (result) {
	$("#page-message-blocked-images-banner").hide();
	clearMessageViewBody ();
	fillMessageViewBody (globalStatus.messageStructure);
    };
    op.onFinish = function () {
	removeOperation (this.opId);
    }
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
    if ('showMessages' in globalStatus.requests) {
	globalStatus.requests["showMessages"].cancel();
	delete globalStatus.requests["showMessages"];
    }

    $("#messages-list-get-more-list").hide();
    $("#messages-list-getting-more-list").show();
    $("#messages-refresh").hide();
    $("#messages-cancel").show();

    retrieveCount = onlyNew?0:SHOW_MESSAGES_COUNT;

    op = iwk.ServiceMgr.fetchMessages (accountId, folderId, retrieveCount,
				       globalStatus.newestUid,
				       globalStatus.oldestUid);
    globalStatus.requests["showMessages"] = op;
    op.opId = addOperation (op, "Fetching messages");
    op.onSuccess = function (result) {
	if (result.new_messages.length > 0) {
	    globalStatus.newestUid = result.new_messages[0].uid;
	}
	if (globalStatus.newestUid == null && result.messages.length > 0) {
	    globalStatus.newestUid = result.messages[0].uid;
	}
	if (result.messages.length > 0) {
	    globalStatus.oldestUid = result.messages[result.messages.length - 1].uid;
	}
	result.new_messages.reverse();
	for (i in result.new_messages) {
	    dumpMessageInMessagesList (result.new_messages[i], true, "#page-messages #messages-list");
	}
	for (i in result.messages) {
	    dumpMessageInMessagesList (result.messages[i], false, "#page-messages #messages-list");
	}
	if ($("#messages-list").hasClass("ui-listview"))
	    $("#messages-list").listview('refresh');
    };
    op.onError = function () {
	showError (this.error.message);
    };
    op.onFinish = function () {
	removeOperation (this.opId);
	if ('showMessages' in globalStatus.requests)
	    delete globalStatus.requests["showMessages"];
	$("#messages-list-get-more-list").show();
	$("#messages-list-getting-more-list").hide();
	$("#messages-refresh").show();
	$("#messages-cancel").hide();
    };
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
    result = iwk.AccountMgr.getAccounts ();
    result.onSuccess = function (result) {
	globalStatus.accounts = result;
	fillComposerFrom (result);
	fillAccountsList (result);
	syncFolders();
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
    result = iwk.AccountMgr.deleteAccount (globalStatus.currentAccount);
    result.onSuccess = function (result) {
	globalStatus.currentFolder = null;
	globalStatus.currentMessage = null;
	refreshAccounts ();
	history.go(-2);
    }
    result.onError = function (result) {
	showError (this.error.message);
    }
}

$(function () {
    $("#page-message-blocked-images-banner").hide();
    refreshAccounts();
});
