/* mainView.js : UI control implementation */

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

var SHOW_MESSAGES_COUNT = 20;

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
		if (!folder.noSelect) {
		    a = document.createElement("a");
		    a.accountId=accountId;
		    a.folderFullName=fullName;
		    a.displayName = folder.displayName;
		    a.setAttribute('href', '#page-messages');
		    $(a).click(function () {
			if (globalStatus.currentAccount != this.accountId || globalStatus.currentFolder != this.folderFullName) {
			    globalStatus.currentAccount = this.accountId;
			    globalStatus.currentFolder = this.folderFullName;
			    globalStatus.currentmessage = null;
			    globalStatus.newestUid = null;
			    globalStatus.oldestUid = null;
			    $("#page-messages-title").text(this.displayName);
			    $("#page-message-title").text(this.displayName);
			    $("#page-messages #messages-list").html("");
			    showMessages(this.accountId, this.folderFullName, false);
			} else {
			    $("#messages-list-get-more-list").show();
			    $("#messages-list-getting-more-list").hide();
			}
			return true;
		    });
		}

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
		if (folder.noSelect) {
		    li.appendChild(h3);
		} else {
		    a.appendChild(h3);
		    li.appendChild(a);
		}
		$("#page-folders #folders-list").append(li);
	    }
	    $("#page-folders #folders-list").listview('refresh');

	    break;
	}
    }
}

function abortShowMessages ()
{
    if ('showMessages' in globalStatus.requests) {
	globalStatus.requests["showMessages"].abort();
    }
}

function weekString (n)
{
    switch (time.getDay()) {
    case 0:
	return "Sunday";
    case 1:
	return "Monday";
    case 2:
	return "Tuesday";
    case 3:
	return "Wednesday";
    case 4:
	return "Thursday";
    case 5:
	return "Friday";
    case 6:
	return "Saturday";
    }
}

function trim(s)
{
    return s.replace(/^\s*|\s*$/g,"");
}

function trimQuotes(s) {
    return s.replace(/^[\s\"]*|[\s\"]*$/g,"");
}

function splitAddressesList (str)
{
    afterAt = false;
    len = str.length;

    if (len == 0)
	return [];
    start = 0;


    while (start < len && 
	   (" ,;\n".match(str[start])))
	start++;

    if (start == len)
	return [];

    end = start + 1;
    while (end < len && str[end] != ';' && !(afterAt && str[0] == ',')) {
	if (str[end] == '"') {
	    Seed.print("Found quote at end "+end);
	    while (end < len && str[end] != '"')
		end++;
	}
	if (str[end] == '@') {
	    afterAt = true;
	}
	if ((end<len && str[end] == '>')&&((end + 1 < len) && str[end+1] == ',')) {
	    end++;
	    break;
	}
	end++;
    }

    return [str.substring(start, end)].concat(splitAddressesList(str.substring(end+1)));
}

function addressGetDisplay(address)
{
    inQuotes = false;
    if (address == null || address.length == 0)
	return "(no recipient)";

    for (i = 0; i < address.length; i++) {
	if (inQuotes) {
	    if (address[i] == '"')
		inQuotes = false;
	} else if (address[i] == '"') {
	    inQuotes = true;
	} else if (address[i] == '<') {
	    return trimQuotes(address.substring (0, i-1));
	}
    }

    return trimQuotes(address);
}

function addressesGetDisplay(str)
{
    if (str == null)
	return "(no recipient)";
    recipients = splitAddressesList (str);
    result = "";
    for (i in recipients) {
	recipients[i] = addressGetDisplay(recipients[i]);
    }
    return recipients.join(", ");
}

function formatTime(t)
{
    current = new Date();
    today = new Date();
    today.setTime(current.getTime());
    today.setHours(0,0,0,0);
    yesterday = new Date();
    yesterday.setHours(0,0,0,0);
    yesterday.setDate(yesterday.getDate() - 1);
    tomorrow = new Date()
    tomorrow.setHours(0,0,0,0);
    tomorrow.setDate(tomorrow.getDate() + 1);
    aWeekAgo = new Date()
    aWeekAgo.setHours(0,0,0,0);
    aWeekAgo.setDate(aWeekAgo.getDate() - 7);

    time = new Date(t*1000);

    if (time >= tomorrow || time < aWeekAgo) {
	return time.toLocaleDateString ();
    } else if (time >= today) {
	minutes = time.getMinutes();
	minutes = minutes + "";
	if (minutes.length == 1) {
	    minutes = "0"+minutes;
	}
	return time.getHours()+":"+minutes;
    } else if (time >= yesterday) {
	return "Yesterday";
    } else if (time >= aWeekAgo) {
	return weekString(time.getDay());
    }
}

function dumpBestAlternative (multipart, parent)
{
    best = -1;
    for (i in multipart.parts) {
	if (multipart.parts[i].contentType.type == 'text') {
	    if (multipart.parts[i].contentType.subType == 'html') {
		best = i;
		break;
	    } else if (multipart.parts[i].contentType.subType == 'plain') {
		best = i;
	    }
	}
    }

    if (best != -1)
	dumpDataWrapper (multipart.parts[i], parent);
}

function dumpMultipart (multipart, parent)
{
    if (multipart.mimeType.subType == 'alternative') {
	dumpBestAlternative (multipart, parent);
    } else if (multipart.mimeType.subType == 'related') {
	if (multipart.parts.length > 0)
	    dumpDataWrapper (multipart.parts[0], parent);
    } else {
	for (i in multipart.parts)
	    dumpDataWrapper (multipart.parts[i], parent);
    }
}

function dumpDataWrapper (dataWrapper, parent)
{
    if (dataWrapper.isMultipart) {
	dumpMultipart (dataWrapper, parent);
    } else if (dataWrapper.isMedium) {
	if (dataWrapper.isMessage) {
	    console.log("Is message, see content (should also add headers)");
	}
	if (dataWrapper.content.isMultipart) {
	    dumpDataWrapper (dataWrapper.content, parent);
	} else {
	    if (dataWrapper.content.mimeType.type == 'text' &&
		(dataWrapper.content.mimeType.subType == 'html' ||
		 dataWrapper.content.mimeType.subType == 'plain') &&
		dataWrapper.disposition != 'attachment') {
		iframe = document.createElement ("iframe");
		iframe.setAttribute ("id", dataWrapper.content.uri);
		iframe.className += "iwk-part-iframe";
		iframe.setAttribute ("border", "0");
		iframe.setAttribute ("frameborder", "0");
		iframe.setAttribute ("src", dataWrapper.content.uri);
		iframe.setAttribute ("width", "100%");
		$(parent).append(iframe);
	    } else {
		p = document.createElement ("p");
		$(p).text("Attachment "+dataWrapper.filename);
		$(parent).append(p);
	    }
	}
    } else {
    }
}

function showMessage(message)
{
    outgoing = message.draft || getCurrentFolder().isSent;
    if (outgoing)
	$("#page-message #recipient").text(addressesGetDisplay (message.to));
    else
	$("#page-message #recipient").text(addressGetDisplay (message.from));
    $("#page-message #subject").text(message.subject);
    $("#page-message #date").text(formatTime (message.dateReceived));
    $("#page-message #iframe-container").text("");
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
	    parent = "#page-message #iframe-container";
	    dumpDataWrapper (msg.result, parent);
	}).always(function(jqXHR, textStatus, errorThrown) {
	    if ('getMessage' in globalStatus.requests)
		delete globalStatus.requests["getMessage"];
	});
}

function createMessageItem (message)
{
    li = document.createElement("li");
    li.setAttribute('data-role', 'fieldcontain');
    if (message.unread)
	li.className += " iwk-unread-item";
    else
	li.className += " iwk-read-item";
    a = document.createElement("a");
    a.setAttribute("href", "#page-message");
    a.accountId = globalStatus.currentAccount;
    a.folderFullName = globalStatus.currentFolder
    a.messageUid = message.uid;
    a.message = message;
    $(a).click(function () {
	globalStatus.currentAccount = this.accountId;
	globalStatus.currentFolder = this.folderFullName
	globalStatus.currentMessage = this.messageUid;
	showMessage(this.message);
	return true;
    });
    h3 = document.createElement("h3");
    $(h3).text(message.subject);
    p = document.createElement("p");
    outgoing = message.draft || getCurrentFolder().isSent;
    if (outgoing) {
	$(p).text(addressesGetDisplay(message.to));
    } else {
	$(p).text(addressGetDisplay(message.from));
    }
    date = document.createElement("p");
    date.className += "ui-li-aside";
    $(date).text(formatTime(message.dateReceived));
    a.appendChild(h3);
    a.appendChild(p);
    a.appendChild(date);
    li.appendChild(a);

    return li;
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
		var message = msg.newMessages[i];
		item = createMessageItem(message);
		$("#page-messages #messages-list").prepend(messageItem);
	    }
	    for (i in msg.messages) {
		var message = msg.messages[i];
		item = createMessageItem(message);
		$("#page-messages #messages-list").append(item);
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
		displayName = parent.displayName + "/" + displayName;
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
	    showFolders(globalStatus.currentAccount);
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
		sync();
	});
	request.fail(function(jqXHR, textStatus) {
	});
	request.error(function(jqXHR, textStatus, errorThrown) {
	});
	request.complete(function(jqXHR, textStatus) {
	});
    } catch (e) {
	console.log(e.message);
    }
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

function updateContentIdFrame (iframeId, height)
{
    $("#iframe-container > [src='"+iframeId+"']").attr("height", 1);
    $("#iframe-container > [src='"+iframeId+"']").attr("height", height + 32);
}

function setIframeHeight( iframeId ) /** IMPORTANT: All framed documents *must* have a DOCTYPE applied **/
{
 var ifDoc, ifRef = document.getElementById( iframeId );

 try
 {   
  ifDoc = ifRef.contentWindow.document.documentElement;  
 }
 catch( e )
 { 
  try
  { 
   ifDoc = ifRef.contentDocument.documentElement;  
  }
  catch(ee)
  {   
  }  
 }
 
 if( ifDoc )
 {
  ifRef.height = 1;  
  ifRef.height = ifDoc.scrollHeight;
  
  /* For width resize, enable below.  */
  
  // ifRef.width = 1;
  // ifRef.width = ifDoc.scrollWidth; 
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
