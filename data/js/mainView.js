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

function fillAccountsListCounts ()
{
    $("#page-accounts #accounts-list").listview();
    
    for (i in globalStatus.folders) {
	account = globalStatus.folders[i];
	accountId = account.accountId;
	inbox = account.folders["INBOX"];
	accountItemId = "account-item-"+accountId;
	$("#page-accounts #account-item-"+accountId+" .account-count").text(inbox.unreadCount);
	if (inbox.unreadCount > 0) {
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

    globalStatus.requests["getMessage"] = $.ajax({
	    type: "GET",
	    crossDomain: true,
	    isLocal: true,
	    dataType: "jsonp",
	    url: "iwk:getMessage",
	    data: {
		account: globalStatus.currentAccount,
		folder: globalStatus.currentFolder,
		message: message.uid
	    }
	}).done(function (msg) {
	    fillMessageViewBody (msg.result);
	}).always(function(jqXHR, textStatus, errorThrown) {
	    if ('getMessage' in globalStatus.requests)
		delete globalStatus.requests["getMessage"];
	});
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
	globalStatus.requests["showMessages"] = $.ajax({
	    type: "GET",
	    crossDomain: true,
	    isLocal: true,
	    dataType: "jsonp",
	    url: "iwk:getMessages",
	    data: {
		account: accountId,
		folder: folderId,
		newestUid: globalStatus.newestUid,
		oldestUid: globalStatus.oldestUid,
		count: retrieveCount
	    }
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
   try {
	var request = $.ajax({
	    type: "GET",
	    crossDomain: true,
	    isLocal: true,
	    dataType: "jsonp",
	    url: "iwk:getAccounts",
	});
	request.done(function (msg) {
	    $("#page-accounts #accounts-list").html("");
	    $("#composer-from-choice").html("");
	    for (i in msg.result) {
		var account = msg.result[i];
		li = document.createElement("li");
		li.setAttribute('id', 'account-item-'+account.id);
		li.setAttribute('data-role', 'fieldcontain');
		a = document.createElement("a");
		a.setAttribute('href', '#page-folders?account='+account.id);
		a.accountId = account.id;
		$(a).click(function () {
		    globalStatus.currentAccount = this.accountId;
		    globalStatus.currentFolder = null;
		    globalStatus.currentMessage = null;
		    fillFoldersList(this.accountId);
		    return true;
		});

		h3 = document.createElement("h3");
		$(h3).text(account.displayName);
		p = document.createElement("p");
		if (account.isDefault)
		    $(p).text(account.emailAddress + " (default)")
		else
		    $(p).text(account.emailAddress);
		countSpan = document.createElement("span");
		countSpan.className += " ui-li-count account-count";
		$(countSpan).hide();
		$(countSpan).text(0);
		a.appendChild(h3);
		a.appendChild(p);
		a.appendChild(countSpan);
		li.appendChild(a);
		$("#page-accounts #accounts-list").append(li);

		composerFromOption=document.createElement("option");
		composerFromOption.setAttribute("id", "composer-from-"+account.id);
		composerFromOption.setAttribute('value', account.id);
		composerFromOption.account = account.id;
		if (account.isDefault)
		    composerFromOption.setAttribute("selected", "true");
		$(composerFromOption).text(account.emailAddress);
		$("#composer-from-choice").append(composerFromOption);
	    }
	    li = document.createElement ("li");
	    a = document.createElement ("a");
	    a.setAttribute("href", "#create-account");
	    a.setAttribute("id", "add-account-button");
	    a.setAttribute("data-role", "button");
	    a.setAttribute("data-icon", "plus");
	    a.className += "account-item";
	    a.innerText = "Add account";

	    li.appendChild(a);
	    $("#page-accounts #accounts-list").append(li);
	    $("#page-accounts #accounts-list").listview('refresh');

	    syncFolders();
	});
	request.fail(function(jqXHR, textStatus) {
	});
	request.error(function(jqXHR, textStatus, errorThrown) {
	});
   } catch (e) {
       console.log(e.message);
   }
}

function addAccount (data)
{
    try {
	$.mobile.showPageLoadingMsg();
	var request = $.ajax({
	    type: "GET",
	    crossDomain: true,
	    isLocal: true,
	    dataType: "jsonp",
	    url: "iwk:addAccount",
	    data: { formData: data}
	});
	request.done(function (msg) {
	    if (msg.is_ok) {
		$.mobile.changePage("#page-accounts");
		refreshAccounts();
	    } else {
		showError (msg.error);
	    }
	});
	request.complete(function(jqXHR, textStatus) {
	    $.mobile.hidePageLoadingMsg();
	});
    } catch (e) {
	console.log(e.message);
    }
}

function syncFolders ()
{
    try {
	$.mobile.showPageLoadingMsg();
	var request = $.ajax({
	    type: "GET",
	    crossDomain: true,
	    isLocal: true,
	    dataType: "jsonp",
	    url: "iwk:syncFolders",
	});
	request.done(function (msg) {
	    globalSetFolders (msg.result);
	    fillAccountsListCounts ();
	    fillFoldersList(globalStatus.currentAccount);
	});
	request.fail(function(jqXHR, textStatus) {
	});
	request.error(function(jqXHR, textStatus, errorThrown) {
	});
	request.complete(function(jqXHR, textStatus) {
	    $.mobile.hidePageLoadingMsg();
	});
    } catch (e) {
	console.log(e.message);
    }
}

function composerSend (data)
{
    try {
	var request = $.ajax({
	    type: "GET",
	    crossDomain: true,
	    isLocal: true,
	    dataType: "jsonp",
	    url: "iwk:composerSend",
	    data: { formData: data}
	});
	request.done(function (msg) {
	    if (!msg.is_ok)
		showError (msg.error);
	    else
		syncFolders();
	});
    } catch (e) {
	console.log(e.message);
    }
}

$(function () {
    $("#form-add-account").submit(function () {
	try {
	    addAccount($(this).serialize());
	} catch (e) {
	    console.log(e.message);
	}
	return false;
    });

    $("#add-account-button").click(function () {
	clearForm($('#form-add-account'));
	return true;
    });

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

    $("#cancel-add-account").click(function () {
	$.mobile.changePage("#page-accounts");
    });

    refreshAccounts();
});
