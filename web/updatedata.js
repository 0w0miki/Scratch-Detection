// 不考虑任何版本IE浏览器的兼容性问题

/*
 * 定义$为css选择器风格选取元素
 */
function $(style){
	'use script';
	if(typeof style != 'undefined'){
		return document.querySelector(style);
	}
}

/*
 * 判断页数是否合理
 * @param {number} pagenum 
 */
function isPageValid(pagenum) {
    console.log("" + pagenum);
    if (isNaN(pagenum)) {
        alert("等待连接服务器");
        return false;
    }else if(isNaN($("#cur_total_page").innerHTML)){
        alert("未获取所有页数，无法设置");
        return false;
    }else if(pagenum > $("#cur_total_page").innerHTML){
        alert("所设置页数大于当前任务打印数");
        return false;
    }
    return true;
}

/**
 * 查看图片
 */
function viewImage() {
    let listItem = document.querySelectorAll('.active>div');
    let str = "";
    let len = listItem.length;
    for (let i = 0; i < len; ++i){
        str += "_" + listItem[i].innerHTML;
    }
    console.log(str);

    $('#origin-img').src = "/images/templates/" + $(".active").getAttribute("origin");
    let filename = "";
    let divs = document.querySelectorAll(".active > div");
    if($(".active").getAttribute("reprint"))
        filename = "reprint_" + divs[0].innerHTML + "_" + divs[1].innerHTML + ".ppm";
    else
        filename = "print_" + divs[0].innerHTML + "_" + divs[1].innerHTML + ".ppm";
    $('#photo').src = "Output/images/" + "scratch" + filename + ".jpg";
    // let xhr = new XMLHttpRequest();
    // xhr.open('GET',"/images/test/print" + str + ".ppm",true);
    // xhr.responseType = "text";
    // xhr.send();
    // xhr.onload = function () {
    //     let ppm = this.response;
    //     displayppm(ppm);
    // }
}

/*
 * canvas中绘制ppm图像 废止不用
 * @param {*} ppmblob ppm的二进制
 */
function displayppm(ppmblob) {
    // Remove lines comment lines beginning with '#', then rejoin.
    let reader = new FileReader();

    let lines = ppmblob.split('\n')
    .filter(l => !l.match(/^#/))
    .join('\n')

    let whitespace = /[ \t\r\n]+/
    let fields = lines.split(whitespace)

    let [magic, width, height, maxVal, ...pixelData] = fields

    if (magic !== 'P6')
    return console.error(`Expected 'P6' magic number at the beginning of the file, got ${magic}`)

    width = parseInt(width, 10)
    if (isNaN(width))
    return console.error(`Expected a decimal value for width, got ${width}`)

    height = parseInt(height, 10)
    if (isNaN(height))
    return console.error(`Expected a decimal value for height, got ${height}`)

    maxVal = parseInt(maxVal, 10)
    if (isNaN(maxVal) || maxVal < 0 || maxVal >= 65536)
    return console.error(`Expected a positive decimal less than 65536 for maximum color value, got ${maxVal}`)

    reader.readAsArrayBuffer(ppmblob);

    let imgData = $("#photo").getContext("2d").createImageData(width, height)
    let pixels = imgData.data

    let to255 = 255 / maxVal
    let pdi, pi, r, g, b, y, x
    for (y = 0, pdi = 0, pi = 0; y < height; ++y) {
        for (x = 0; x < width; ++x, pdi += 3, pi += 4) {
            r = parseInt(pixelData[pdi])
            if (isNaN(r)) return console.error(`Expected a decimal value for red component, got ${r}`)
            g = parseInt(pixelData[pdi + 1])
            if (isNaN(g)) return console.error(`Expected a decimal value for green component, got ${g}`)
            b = parseInt(pixelData[pdi + 2])
            if (isNaN(b)) return console.error(`Expected a decimal value for blue component, got ${b}`)

            r *= to255
            g *= to255
            b *= to255
            pixels.set([r,g,b,255], pi)
        }
    }
}

function setTask() {
    // 发送get请求更新后台的页数
    let a = document.querySelectorAll('.active>div');
    let id = a[0].innerHTML;
    let pagenum = Number(a[1].innerHTML);
    let index = Array.from(document.querySelectorAll('.task-item')).indexOf($('.active'));
    if(isPageValid(pagenum)){
        let xhr = new XMLHttpRequest();
        xhr.open('POST', 'api/setTask');
        xhr.responseType = "text";
        xhr.setRequestHeader("Content-type","application/json");
        let msg = '{"id":' + id + ',"page":' + pagenum + ',"index":' + index + '}';
        xhr.send(msg);
    }
}

/*
 * 收到服务器端信息更新当前处理作业
 * @param {number} index 序号
 */
function updateIndex(index) {
    let items = document.querySelectorAll('.task-item');
    let lastItem = $('.task-item.cur');
    
    // 滚动到视野适当位置
    if(lastItem && lastItem.classList.contains('active')){
        lastItem.classList.remove('active');
        items[index-1].classList.add('active');
    }
    lastItem.classList.remove('cur');
    items[index-1].classList.add('cur');
    changeView(index);
}
/*
 * 更新视野
 */
function changeView() {
    let unitHeight = $('.task-item').offsetHeight;
    let index = Array.from(document.querySelectorAll('.task-item')).indexOf($('.active'));
    $('ul').scrollTo(0,(index-3)*unitHeight);
}

/*
 * 读取服务器发来的作业信息并更新ul列表
 * @param {string} rcv 收到的信息
 */
function readWorkList(rcv){
    let ul = $("ul");
    ul.innerHTML = "";
    if(!(rcv["list"])){
        let li = '<li class="task-item">';
        li += '<div>尚未开始</div>';
        li += '<div>'+'</div>';
        li += '</li>';
        ul.innerHTML += li;
        return;
    }


    for (let i = 0; i < rcv["list"].length; i++) {
        const element = rcv["list"][i];
        if(element.is_reprint){
            let li = '<li class="task-item" reprint>';
            li += '<div>' + element.id + '</div>';
            li += '<div>' + element.num + '</div>';
            li += '</li>';
            ul.innerHTML += li;
        }else{
            let page = 1;
            for (let j = 0; j < element.num; j++) {
                let li = '<li class="task-item">';
                li += '<div>' + element.id + '</div>';
                li += '<div>' + page + '</div>';
                li += '</li>';
                ul.innerHTML += li;
                page++;
            }
        }
    }
}

/*
 * 切换相机触发方式
 */
function changeMode(){
    let xhr = new XMLHttpRequest();
    xhr.open('POST', 'api/changeMode');
    xhr.responseType = "text";
    let msg, info;
    switch ($("#trigger_mode").innerHTML) {
        case "硬触发":
            msg = "soft";
            info = "软触发"
            break;
        case "软触发":
            msg = "hard";
            info = "硬触发"
            break;
        default:
            break;
    }
    xhr.send(msg);
    xhr.onreadystatechange=function(){
        if(xhr.readyState == 4 && xhr.status == 200)
        {
            console.log(xhr.response);
            if(xhr.response == "done"){
                $("#trigger_mode").innerHTML = info;
                if(info == "软触发")
                {
                    $("#trigger_button").removeAttribute("disabled");
                    $("#trigger_button").addEventListener("click",trigger);
                }else{
                    $("#trigger_button").setAttribute("disabled",true);
                    $("#trigger_button").removeEventListener("click",trigger);
                }
            }
        }
    }
}

/**
 * 触发相机拍摄
 */
function trigger(){
    let xhr = new XMLHttpRequest();
    xhr.open('GET', 'api/softTrigger');
    xhr.responseType = "text";
    xhr.send();
}

/*
 * 读取服务器发来的原图信息并更新列表项属性
 * @param {string} rcv 收到的信息
 */
function readOriginList(rcv){
    let index = 0;
    let workList = document.querySelectorAll(".task-item");
    for (let i = 0; i < rcv["origin"].length; i++){
        const element = rcv["origin"][i];
        for (let j = 0; j < element.num; j++){
            workList[index].setAttribute("origin",element.pos);
            index++;
        }
    }
}

/*
 * 判断是否是当前状态信息
 * @param {string} str 
 */
function isState(str) {
    let reg = /^statemsg/;
    return reg.test(str);
}

/*
 * 判断是否是列表信息
 * @param {string} str 
 */
function isList(str) {
    let reg = /^listmsg/;
    return reg.test(str);
}

/*
 * 判断是否是相机触发状态信息
 * @param {string} str 
 */
function isTrigger(str) {
    let reg = /^trigger/;
    return reg.test(str);
}

// 添加按钮回调函数
$('#page_button').addEventListener(
    "click", setTask
)

$('#img_button').addEventListener(
    'click', viewImage
)

$('#mode_button').addEventListener(
    "click", changeMode
)

// 打开一个 web socket
var ws = new WebSocket("ws://localhost:7999/websocket");

ws.onopen = function()
{
    // Web Socket 已连接上，使用 send() 方法发送数据
    ws.send("发送数据");
    console.log("数据发送中...");
};
// websocket接收消息回调
ws.onmessage = function (evt) 
{
    let received_msg = evt.data;
    if(isState(received_msg)){
        received_msg = received_msg.replace(/^statemsg/,"");
        let rcv = JSON.parse(received_msg);
        console.log(rcv);
        $("#cur_work_id").innerHTML = rcv["id"];
        $("#cur_detect").innerHTML = rcv["page"];
        $("#cur_total_page").innerHTML = rcv["totalpage"];
        let index = rcv["index"];
        updateIndex(index);
    }else if(isList(received_msg)){
        let workArray = Array.from(document.querySelectorAll('.task-item'));
        let select_index = workArray.indexOf($('.active'));
        let index = workArray.indexOf($('.cur'));
        console.log("select:"+select_index+",index:"+index);

        received_msg = received_msg.replace(/^listmsg/,"");
        let rcv = JSON.parse(received_msg);
        console.log(rcv);
        readWorkList(rcv);
        readOriginList(rcv);

        let items = document.querySelectorAll(".task-item");
        if (index != rcv["index"] - 1) {
            select_index -= index - (rcv["index"] - 1);
            index = rcv["index"] - 1;
        }
        items[select_index].classList.add("active");
        items[index].classList.add("cur");
        changeView(index);
    }else if(isTrigger(received_msg)){
        received_msg = received_msg.replace(/^trigger:/,"");
        console.log("trigger:"+received_msg);
        if(Number(received_msg)){
            $("#trigger_mode").innerHTML = "软触发";
            $("#trigger_button").removeAttribute("disabled");
        }
        else{
            $("#trigger_mode").innerHTML = "硬触发";
            $("#trigger_button").setAttribute("disabled",true);
        }
    }else{
        console.log("接收数据.."+received_msg);
    }
};
// websocket关闭链接回调
ws.onclose = function()
{ 
    // 关闭 websocket
    console.log("连接已关闭..."); 
};

// 列表项点击回调
var ele = $('ul');
ele.addEventListener('click', function(e){
    var element = e.target;
    if(!$('.task-item.active'))
        return;
    if(element.classList.contains('task-item')){
        $('.task-item.active').classList.remove('active');
        element.classList.add('active');
        // $('#origin-img').src = "/images/templates/" + element.getAttribute("origin");
        console.log('click li');
    }else if(element.parentNode.classList.contains('task-item')){
        $('.task-item.active').classList.remove('active');
        element.parentNode.classList.add('active');
        // $('#origin-img').src = "/images/templates/" + element.parentNode.getAttribute("origin");
        console.log('click div');
    }
})