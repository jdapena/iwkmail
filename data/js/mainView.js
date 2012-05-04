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

function fillComposerFrom (accounts)
{
    $("#composer-from-choice").html("");
    for (i in accounts) {
	dumpAccountOptionInComposerFrom (accounts[i], "#composer-from-choice");
    }
}

function fillAccountsList (accounts)
{
    clearAccountsList ();

    for (i in accounts) {
	dumpAccountInAccountsList (accounts[i], "#page-accounts #accounts-list");
    }
    dumpAddAccountInAccountsList ("#page-accounts #accounts-list");

    $("#page-accounts #accounts-list").listview('refresh');
}

function fillAccountsListCounts ()
{
    $("#page-accounts #accounts-list").listview();
    
    for (i in globalStatus.folders) {
	account = globalStatus.folders[i];
	accountId = account.accountId;
	inbox = account.folders["INBOX"];
	if (!inbox)
	    inbox = account.folders["inbox"];
	accountItemId = "account-item-"+accountId;
	$("#page-accounts #account-item-"+accountId+" .account-count").text(inbox.unreadCount);
	if (inbox && inbox.unreadCount > 0) {
	    $("#page-accounts #account-item-"+accountId+" .account-count").show();
	} else {
	    $("#page-accounts #account-item-"+accountId+" .account-count").hide();
	}
    }
    $("#page-accounts #accounts-list").listview('refresh');
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
	    $("#page-folders #folders-list").listview('refresh');

	    break;
	}
    }
}

function fillMessageViewHeader (message)
{
    dumpMessageAsViewHeader (message, "#page-message");
}

function fillMessageViewBody (message)
{
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

function showMessage(message)
{
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
	    if (msg.newMessages.length > 0) {
		globalStatus.newestUid = msg.newMessages[0].uid;
	    }
	    if (globalStatus.newestUid == null && msg.messages.length > 0) {
		globalStatus.newestUid = msg.messages[0].uid;
	    }
	    if (msg.messages.length > 0) {
		globalStatus.oldestUid = msg.messages[msg.messages.length - 1].uid;
	    }
	    msg.newMessages.reverse();
	    for (i in msg.newMessages) {
		dumpMessageInMessagesList (msg.newMessages[i], true, "#page-messages #messages-list");
	    }
	    for (i in msg.messages) {
		dumpMessageInMessagesList (msg.messages[i], false, "#page-messages #messages-list");
	    }
	    $("#page-messages #messages-list").listview('refresh');
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
    showMessages (globalStatus.currentAccount, globalStatus.currentFolder, true);
}

function refreshAccounts ()
{
    iwkRequest ("getAccounts", "Updating accounts", {
    }).done(function (msg) {
	fillComposerFrom (msg.result);
	fillAccountsList (msg.result);
	syncFolders();
    });
}

function syncFolders ()
{
    iwkRequest("syncFolders", "Updating accounts and folders", {
    }).done(function (msg) {
	globalSetFolders (msg.result);
	fillAccountsListCounts ();
	fillFoldersList(globalStatus.currentAccount);
    });
}

function composerSend (data)
{
    iwkRequest ("composerSend", "Adding to outbox", {
	formData: data
    }).done(function (msg) {
	if (!msg.is_ok)
	    showError (msg.error);
	else
	    syncFolders();
    });
}

$(function () {
    $("#composer-send").click(function () {
	$("#form-composer").submit();
    });

    $("#form-composer").submit(function () {
	composerSend($(this).serialize());
	return false;
    });

    $("#accounts-compose").click(function () {
	clearForm($('#form-composer'));
	return true;
    });

    refreshAccounts();
});
