/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

/* im-error.h : Error handling */

/*
 * Authors:
 *  Jose Dapena Paz <jdapena@igalia.com>
 *
 * Copyright (c) 2012, Igalia, S.L.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * * Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * * Neither the name of the Nokia Corporation nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

var globalStatus = {
    currentAccount: null,
    folders: [],
};

function showError (e)
{
    $("#error-dialog #error-message").text(e);
    $.mobile.changePage("#error-dialog");
}

function refreshAccountCounts ()
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

function showFolders(accountId)
{
    for (i in globalStatus.folders) {
	account = globalStatus.folders[i];
	if (account.accountId == accountId) {
	    $("#page-folders-title").text(account.accountName + " folders");

	    $("#page-folders #folders-list").html("");
	    for (fullName in account.folders) {
		
		folder = account.folders[fullName];

		li = document.createElement("li");
		li.setAttribute('id', 'folder-item-'+accountId+'-'+fullName);
		li.setAttribute('data-role', 'fieldcontain');
		a = document.createElement("a");

		h3 = document.createElement("h3");
		if ('fullDisplayName' in folder)
		    $(h3).text(folder.fullDisplayName);
		else
		    $(h3).text(folder.displayName);
		unreadCount = 0;
		if ('unreadCount' in folder && folder.unreadCount > 0) {
		    countSpan = document.createElement("span");
		    countSpan.className += " ui-li-count";
		    $(countSpan).text(folder.unreadCount);
		    a.appendChild(countSpan);
		    unreadCount = folder.unreadCount;
		}
		if (fullName == "INBOX") {
		    accountItemId = "account-item-"+accountId;
		    $("#page-accounts #"+accountItemId+" .countSpan:first").text(unreadCount);
		    if (unreadCount > 0)
			$("#page-accounts #"+accountItemId+" .countSpan:first").show();
		    else
			$("#page-accounts #"+accountItemId+" .countSpan:first").hide();
		}
		a.appendChild(h3);
		li.appendChild(a);
		$("#page-folders #folders-list").append(li);
	    }
	    $("#page-folders #folders-list").listview('refresh');

	    break;
	}
    }
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
		    showFolders(this.accountId);
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

	    sync();
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

function updateDisplayNames ()
{
    for (i in globalStatus.folders) {
	account = globalStatus.folders[i];
	for (folderFullName in account.folders) {
	    folder = account.folders[folderFullName];
	    displayName = folder.displayName;
	    parent = folder;
	    while ('parentFullName' in parent) {
		parent = account.folders[parent.parentFullName];
		displayName = parent.fullName + "/" + displayName;
	    }
	    folder.fullDisplayName = displayName;
	}
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
	    globalStatus.folders = msg.result;
	    updateDisplayNames();
	    refreshAccountCounts ();
	    showFolders(globalStatus.currentAccountId);
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

function sync ()
{
    syncFolders ();
}

function clearForm(form)
{
    $(":input", form).each(function()
    {
	var type = this.type;
	var tag = this.tagName.toLowerCase();
        if (type == 'text')
        {
            this.value = "";
        }
    });
};

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

    $("#cancel-add-account").click(function () {
	$.mobile.changePage("#page-accounts");
    });

    refreshAccounts();
});
