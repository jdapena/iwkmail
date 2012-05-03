/***************************************************************************
 * htmlUtils.js : Helpers for working with HTML
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

