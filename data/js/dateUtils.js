/***************************************************************************
 * dateUtils.js : Helpers for formatting dates
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


function weekString (n)
{
    switch (n.getDay()) {
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
	return weekString(time);
    }
}

