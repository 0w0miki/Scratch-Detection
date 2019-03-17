// 不考虑任何版本IE浏览器的兼容性问题

function $(id){
	'use script';
	if(typeof id != 'undefined'){
		return document.getElementById(id);
	}
}

function selector(style){
	'use script';
	if(typeof style != 'undefined'){
		return document.querySelector(style);
	}
}

function isPageValid(pagenum) {
    console.log("" + pagenum);
    if (isNaN(pagenum)) {
        alert("等待连接服务器");
        return false;
    }else if(isNaN($("cur_total_page").innerHTML)){
        alert("未获取所有页数，无法设置");
        return false;
    }else if(pagenum > $("cur_total_page").innerHTML){
        alert("所设置页数大于当前任务打印数");
        return false;
    }
    return true;
}

function setImage() {
    var listItem = document.querySelectorAll('.active>div');
    var str = "";
    var len = listItem.length;
    for (var i = 0; i < len; ++i){
        str += listItem[i].innerHTML;
    }
    console.log(str);
    
    $('origin-img').src = "0001.jpg";
    $('photo').src = "";
}

function setPage() {
    // 发送get请求更新后台的页数
    var xhr = new XMLHttpRequest();
    var pagenum = Number(document.querySelector('.active').innerHTML);
    if(isPageValid(pagenum)){
        xhr.open('GET', 'api/setpage?page='+pagenum);
        xhr.send();
    }
}

function refreshPage() {
    var xhr = new XMLHttpRequest();
    xhr.open('GET', 'api/refreshdata');

    // xhr.send();
    
    // 异步获取当前的workid和页数 更新页面
    xhr.onreadystatechange=function(){
        if(xhr.readyState==4){
            if(xhr.status=200){
                var data=JSON.parse(xhr.responseText); //json解析方法JSON.parse 或者 eval('('+xhr.responseText+')')
                // NOTE for test
                console.log(data);
                // $('cur_work_id').innerHTML = data.msg;
                // $('cur_page');
                // $('cur_total_page');
            }
        }
    }
}

$('page_button').addEventListener(
    "click", setPage
)

$('img_button').addEventListener(
    'click', setImage
)

window.setInterval(function(){
    refreshPage()
}, 300);

var ele = selector('ul');
ele.addEventListener('click', function(e){
    var element = e.target;
    if(element.classList.contains('task-item')){
        selector('.task-item.active').classList.remove('active');
        element.classList.add('active');
        console.log('click li');
    }else if(element.parentNode.classList.contains('task-item')){
        selector('.task-item.active').classList.remove('active');
        element.parentNode.classList.add('active');
        console.log('click div');
    }
})