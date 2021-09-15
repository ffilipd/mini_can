const ws = new WebSocket('ws://192.168.132.4:3000');

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

    // Mouse button is clicked
    // function putPoint(e) {
    //     if (dragging) {
    //         let data = e.offsetX + ',' + e.offsetY;
    //         ws.send(data)
    //     }
    // }

    function putPoint(e) {
        if (dragging) {
            let data = '';
            if (e.clientX) {
                data = (e.clientX + -10) + ',' + (e.clientY -30);
            }
            else {
                data = e.offsetX + ',' + e.offsetY;
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
        if (msg.data == 'rtn') {
            dragging = false;
            context.beginPath();
        }
        // console.log(msg.data);
        const e = {
            offsetX: parseInt(msg.data.split(',')[0]),
            offsetY: parseInt(msg.data.split(',')[1])
        };
        draw(e);

    };

})
