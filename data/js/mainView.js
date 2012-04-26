var globalStatus = {
    currentAccount: null,
    folders: [],
};

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
		if ('unreadCount' in folder && folder.unreadCount > 0) {
		    countSpan = document.createElement("span");
		    countSpan.className += " ui-li-count";
		    $(countSpan).text(folder.unreadCount);
		    a.appendChild(countSpan);
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
		a.appendChild(h3);
		a.appendChild(p);
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
	var request = $.ajax({
	    type: "GET",
	    crossDomain: true,
	    isLocal: true,
	    dataType: "jsonp",
	    url: "iwk:addAccount",
	    data: { formData: data}
	});
	request.done(function (msg) {
	    refreshAccounts();
	});
	request.fail(function(jqXHR, textStatus) {
	});
	request.error(function(jqXHR, textStatus, errorThrown) {
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
	    console.log(JSON.stringify (msg.result));
	    updateDisplayNames();
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
	    account = {
		name: $("#accountname", this).val(),
		fullname: $("#fullname", this).val(),
		emailaddress: $("#emailaddress", this).val()
		
	    };

	    addAccount($(this).serialize());
	} catch (e) {
	    console.log(e.message);
	}
	$.mobile.changePage("#page-accounts");
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
    sync ();
});