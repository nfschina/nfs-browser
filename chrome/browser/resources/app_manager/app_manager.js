cr.define('appManager', function() {

var RecommendApps = [{
	crx_name: 'Adblock.crx',
	name: loadTimeData.getString('adblock_name'),
	id:"ehlpchdbinckoglkcpaabnfphpkjjfbk",
	path:"img/adblock.png",
	description:loadTimeData.getString('adblock_description')
},{
	crx_name: 'weChat_1.0.0.2018.crx',
	name: loadTimeData.getString('weChat_name'),
	id:"pbokkbikokbcmkgjkdnmoenofdkhphfh",
	path:"img/weChat.png",
	description: loadTimeData.getString('weChat_description')
},{
	crx_name: 'ReadMode.crx',
	name:loadTimeData.getString('readingMode_name'),
	id:"anibhbkbildpjkceocooefadelkpmhkj",
	path:"img/ReadMode.png",
	description: loadTimeData.getString('readingMode_description')
},
{
	crx_name: 'ActiveX.crx',
	name:loadTimeData.getString('activeX_name'),
	id:"pmbohfpjkcmginfmpkgoiojhcejfnaha",
	path:"img/activeX.png",
	description: loadTimeData.getString('activeX_description')
},
{
	crx_name: 'SmoothScroll.crx',
	name:loadTimeData.getString('smoothScroll_name'),
	id:"pbffjimnicgjgooddacophccjkepifgk",
	path:"img/smoothScroll.png",
	description: loadTimeData.getString('smoothScroll_description')
},
{
	crx_name: 'MouseGesture.crx',
	name:loadTimeData.getString('mousegesture_name'),
	id:"dpidckcehhmlmdliiappkbgfobkmcekn",
	path:"img/mousegesture.png",
	description: loadTimeData.getString('mousegesture_description')
},
{
	crx_name: 'pepperFlash',
	name:loadTimeData.getString('pepperFlash_name'),
	id:"pepperFlash",
	path:"img/flash.png",
	description: loadTimeData.getString('pepperFlash_description')
},
];

var targetItem;
var menuList = document.querySelector("#menu-list");
var additem = document.querySelector("#install .add");
var installList = document.querySelector("#install ul");
var menuBtn = document.querySelectorAll("#menu-list li");
var listBtn = document.querySelectorAll("header h4 span");
var moreBtn = document.querySelector("footer h4 a");
var addBtn = document.querySelector("#install .add a");

menuList.addEventListener('click',quitMenu);
menuBtn[0].addEventListener('click',usedItem);
menuBtn[1].addEventListener('click',stopItem);
menuBtn[2].addEventListener('click',dumpItem);
listBtn[0].addEventListener('click',switchInstall);
listBtn[1].addEventListener('click',switchRecommend);
moreBtn.addEventListener('click',onMoreClicked);
addBtn.addEventListener('click',onMoreClicked);

function addItem(item) {
	var icon_url = 'url(' + item.iconUrl + ')';
	let li = document.createElement("li");
	let list = '<dl><dd id="list_item"></dd><dt>' + item.name + '</dt></dl>';
	li.innerHTML = list;
	li.dataset.insid = item.id;
	if(item.state == 'DISABLED'){
		li.dataset.disable = true;
	}
	var list_item = li.querySelector('#list_item');
	list_item.style.backgroundImage = 'url(' + item.iconUrl + ')';
	list_item.style.backgroundRepeat = 'no-repeat';
	list_item.style.backgroundPosition = 'center';
	list_item.style.backgroundSize = '48px 48px';
	installList.insertBefore(li,additem);
	li.addEventListener('mouseenter',editInstallItem);
	li.addEventListener('mouseleave',leaveInstallItem);
	li.addEventListener('mouseup',menuInstallItem);
}

function addItem2(item) {
	let li = document.createElement("li");
	let list = '<dl><dd id="list_item"></dd><dt>' + item.name + '</dt></dl>';
	li.innerHTML = list;
	li.dataset.insid = item.id;
	var list_item = li.querySelector('#list_item');
	list_item.style.backgroundImage = 'url(' + item.path + ')';
	list_item.style.backgroundRepeat = 'no-repeat';
	list_item.style.backgroundPosition = 'center';
	list_item.style.backgroundSize = '48px 48px';
	installList.insertBefore(li,additem);
	li.addEventListener('mouseenter',editInstallItem);
	li.addEventListener('mouseleave',leaveInstallItem);
	li.addEventListener('mouseup',menuInstallItem);
}

function quitMenu() {
	targetItem = null;
	menuList.style.display = "none";
}

//启用
function usedItem() {
	var id = targetItem.dataset.insid;
	chrome.send('enable', [id]);
}

//停用
function stopItem() {
	var id = targetItem.dataset.insid;
	chrome.send('disable', [id]);
}

//卸载
function dumpItem() {
	var id = targetItem.dataset.insid;
	chrome.send('uninstall', [id]);
}

function installItem(event) {
	if(this.innerText != loadTimeData.getString("install")) {
		return;
	}
	this.innerText = loadTimeData.getString("installing");
	var li = event.path[2];
	var crx_name = li.dataset.crx_name;
	chrome.send('install', [crx_name]);
}

function completeItem(id){
	document.querySelector('[data-recid="' + id + '"]').innerText = loadTimeData.getString("installed");
}

function switchInstall() {
	if(!this.dataset.sld) {
		this.dataset.sld = true;
		delete this.nextSibling.dataset.sld;
		document.querySelector("#install").style.display = "block";
		document.querySelector("#recommend").style.display = "none";
	}
}

function switchRecommend() {
	if(!this.dataset.sld) {
		this.dataset.sld = true;
		delete this.previousSibling.dataset.sld
		document.querySelector("#install").style.display = "none";
		document.querySelector("#recommend").style.display = "block";
	}
}

function onMoreClicked() {
	chrome.send('more', []);
}

function editInstallItem() {
	var text = this.querySelector("dt").innerText
	this.className = "edit";
	this.dataset.name = text;
	this.querySelector("dt").innerText = loadTimeData.getString("setting");;
}

function leaveInstallItem() {
	this.className = "";
	this.querySelector("dt").innerText = this.dataset.name;
}

function menuInstallItem(event) {
	var tag = event.target.nodeName;
	var menuCont = document.querySelector("#menu-list ul"),left;

	console.log(tag);
	console.log(event.path);

	if(tag == "DD"){
		targetItem = event.path[2];
	}
	if(tag == "DT"){
		targetItem = event.path[2];
	}
	if(event.clientX > 380){
		left = event.clientX -150;
	}else{
		left = event.clientX;
	}

	if(!targetItem.dataset.disable){
		menuCont.children[0].style.display = "none";
		menuCont.children[1].style.display = "block";
	}else{
		menuCont.children[1].style.display = "none";
		menuCont.children[0].style.display = "block";
	}

	let id = targetItem.dataset.insid;
	//enable/disable menu item is not available for "pepper flash"
	if (id == "pepperFlash") {
		menuCont.children[0].style.display = "none";
		menuCont.children[1].style.display = "none";
	}

	menuCont.style.top = event.clientY + "px";
	menuCont.style.left = left + "px" ;
	menuList.style.display = "block";
}

//添加安装
function addInstallItem(apps) {
	for(var x in apps) {
		var i = x;
		addItem(apps[x]);
	}
}

//添加推荐
function addRecommendItem(apps) {
	var recommendList = document.querySelector("#recommend ul");

	for(var x in apps) {
		let li = document.createElement("li");
		let list = '<div class="ico"><img src=' + apps[x].path + '></div><div class="des"><h6>' + apps[x].name + '</h6><p title=' + apps[x].description + '>' + apps[x].description + '</p></div><div class="opr"><span data-recid=' + apps[x].id + '>' + loadTimeData.getString("install") + '</span></div>';
		li.innerHTML = list;
		li.dataset.crx_name = apps[x].crx_name;
		recommendList.appendChild(li);
		li.querySelector("span").addEventListener('click',installItem);
	}
}

 /**
   * Compares two extensions for the order they should appear in the list.
   * @param {chrome.developerPrivate.ExtensionInfo} a The first extension.
   * @param {chrome.developerPrivate.ExtensionInfo} b The second extension.
   * returns {number} -1 if A comes before B, 1 if A comes after B, 0 if equal.
   */
  function compareExtensions(a, b) {
    function compare(x, y) {
      return x < y ? -1 : (x > y ? 1 : 0);
    }
    return compare(a.name.toLowerCase(), b.name.toLowerCase());
   }

function onLoad() {
	chrome.send('getExtensionsInfo', []);
}

function onInfosGenerated(extensions) {
	var extensions_ = extensions[0];
	extensions_.sort(compareExtensions);
	addInstallItem(extensions_);
	var apps = new Array();
	for(var i in RecommendApps) {
		for(var j in extensions_) {
			if (RecommendApps[i].id == extensions_[j].id ||
				RecommendApps[i].id == "pepperFlash") {
				break;
			}
			if (j == extensions_.length - 1) {
				apps.push(RecommendApps[i]);
			}
		}
	}
	addRecommendItem(apps);
}

function onInstalled(extension) {
	var item = extension[0][0];
	addItem(item);
	for(var i in RecommendApps) {
		if (item.id == RecommendApps[i].id) {
			completeItem(item.id);
			break;
		}
	}
}

function onUninstalled(id) {
	var li = document.querySelector("[data-insid=" + id +  "]");
	li.parentNode.removeChild(li);
	for(var i in RecommendApps) {
		if(id == RecommendApps[i].id) {
			var item = null;
			if(item = document.querySelector('[data-recid="' + id + '"]')) {
				item.innerText = loadTimeData.getString("install");;
			} else {
				addRecommendItem([RecommendApps[i]]);
			}
		}
	}
}

function onEnabled(extension) {
	var item = extension[0][0];
	var li = document.querySelector("[data-insid=" + item.id +  "]");
	delete li.dataset.disable;
	var list_item = li.querySelector('#list_item');
	list_item.style.backgroundImage = 'url(' + item.iconUrl + ')';
}

function onDisabled(extension) {
	var item = extension[0][0];
	var li = document.querySelector("[data-insid=" + item.id +  "]");
	li.dataset.disable = true;
	var list_item = li.querySelector('#list_item');
	list_item.style.backgroundImage = 'url(' + item.iconUrl + ')';
}

function onPepperFlashInstalled(info) {
	if (info.success) {
		completeItem("pepperFlash");
		for (let i in RecommendApps) {
			if (RecommendApps[i].id == 'pepperFlash') {
				addItem2(RecommendApps[i]);
				break;
			}
		}
	} else {
		console.log("Install failed.");
		document.querySelector('[data-recid="pepperFlash"]').innerText = loadTimeData.getString("install");
	}
}

function OnInstallstatus(status) {
	if (status.id == "pepperFlash") {
		let apps = new Array;
		for (let i in RecommendApps) {
			if (RecommendApps[i].id == 'pepperFlash') {
				apps.push(RecommendApps[i]);
				break;
			}
		}
		if(status.has_installed) {
			addItem2(apps[0]);
		} else {
			addRecommendItem(apps);
		}

	}
}

return {
	onLoad : onLoad,
	onInfosGenerated : onInfosGenerated,
	onInstalled : onInstalled,
	onUninstalled : onUninstalled,
	onEnabled : onEnabled,
	onDisabled : onDisabled,
	onPepperFlashInstalled: onPepperFlashInstalled,
	OnInstallstatus:OnInstallstatus
}
});

document.oncontextmenu = function() {
	return false;
}

document.addEventListener('DOMContentLoaded', appManager.onLoad);
