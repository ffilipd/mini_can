// const ws = new WebSocket('ws://canvas.dahlskog.fi');
const ws = new WebSocket('ws://192.168.132.3:3000');

ws.onopen = function () {
    console.log('CONNECT');
};
ws.onclose = function () {
    console.log('DISCONNECT');
};
ws.onmessage = function (event) {
    // console.log('MESSAGE: ' + event.data);
};


window.addEventListener('load', function () {
    let canvas = document.getElementById('canvas');
    let context = canvas.getContext('2d');

    let radius = 1; // Radius of point in drawing line
    let start = 0; // Start point of arc
    let end = Math.PI * 2;  // End point of arc
    let dragging = false;

    class Client {
        constructor(id, lastX, lastY) {
            this.id = id;
            this.lastX = lastX;
            this.lastY = lastY;
        }
    }

    let clients = [];

    canvas.width = window.innerWidth;
    canvas.height = window.innerHeight;

    context.lineWidth = radius * 2; // Make line same width as points


    const draw = (e) => {
        context.lineTo(e.offsetX, e.offsetY);
        context.stroke();
        context.beginPath();
        context.arc(e.offsetX, e.offsetY, radius, start, end);
        context.fill();
        context.beginPath();
        context.moveTo(e.offsetX, e.offsetY);
    }

    function putPoint(e) {
        if (dragging) {
            let data = '';
            if (e.clientX) {
                data = (e.clientX + -10).toFixed() + ',' + (e.clientY -30).toFixed();
            }
            else {
                data = e.offsetX.toFixed() + ',' + e.offsetY.toFixed();
            }
            ws.send(data)
        }
    }


    // Mouse is dragged with button down
    function engage(e) {
        dragging = true;
        putPoint(e);
    }
    // Mouse button is released
    function disengage() {
        dragging = false;
        context.beginPath();
        ws.send("rtn");
    }

    canvas.addEventListener('mousedown', engage);
    canvas.addEventListener('mousemove', putPoint);
    canvas.addEventListener('mouseup', disengage);


    function touchstart(event) { engage(event.touches[0]) }
    function touchmove(event) { putPoint(event.touches[0]); event.preventDefault(); }
    function touchend(event) { disengage(event.changedTouches[0]) }

    canvas.addEventListener('touchstart', touchstart, false);
    canvas.addEventListener('touchmove', touchmove, false);
    canvas.addEventListener('touchend', touchend, false);




    // Reload page to reset canvas
    document.getElementById('resetbtn').addEventListener('click', function () {
        context.clearRect(0, 0, canvas.width, canvas.height);
        // location.reload();
    })



    ws.onmessage = (msg) => {
        if (msg.data.includes(':')) {
            let splitString = msg.data.split(':');
            let id = splitString[0];
            let message = '';

            if (splitString[1].includes(',')) {
                message = splitString[1].split(',');
            }

            let x = parseInt(message[0]);
            let y = parseInt(message[1]);


            let index = clients.findIndex((c) => { return c.id == id });

            if (index != -1) {
                const e = {
                    offsetX: x,
                    offsetY: y
                };
                if (clients[index].lastX != '') {
                    context.moveTo(clients[index].lastX, clients[index].lastY);
                    draw(e);
                }
                if (clients[index].lastX == '') {
                    context.moveTo(x, y)
                    draw(e);
                }
                clients[index].lastX = x;
                clients[index].lastY = y;

                if (splitString[1] == 'rtn') {
                    clients[index].lastX = '';
                    clients[index].lastY = '';
                }
            }
            else clients.push(new Client(id));
        }


    };
})


