
let lines = [];

function run() {
    const elem = document.getElementById('canvas'),
        context = elem.getContext('2d');

    const overlay = document.getElementById('overlay'),
        overlay_ctx = overlay.getContext('2d');





    const canvasOffsetX = elem.offsetLeft;
    const canvasOffsetY = elem.offsetTop;

    // elem.width = window.innerWidth - canvasOffsetX;
    // elem.height = window.innerHeight - canvasOffsetY;
    // overlay.width = window.innerWidth - canvasOffsetX;
    // overlay.height = window.innerHeight - canvasOffsetY;

    let width = elem.width
    let height = elem.height
    let startx, starty;
    overlay_ctx.strokeStyle = '#000000';

    function clear() {

        context.clearRect(0, 0, elem.width, elem.height);

        const x_size = 10;     // draw a X in the center
        context.strokeStyle = '#FF0000';
        context.beginPath()
        context.moveTo(width / 2, height / 2 + x_size);
        context.lineTo(width / 2, height / 2 - x_size);
        context.moveTo(width / 2 - x_size, height / 2);
        context.lineTo(width / 2 + x_size, height / 2);
        context.stroke();

        context.strokeStyle = '#000000';
        startx = elem.width / 2;
        starty = elem.height / 2;
    }
    clear();

    let mouse_moving = false;
    overlay.addEventListener('mousedown', function(e) {
        mouse_moving = true;
        context.beginPath();
        context.moveTo(startx, starty);
        context.lineTo(e.layerX, e.layerY);
        context.stroke();

        lines.push([[startx, starty], [e.layerX, e.layerY]]);

        startx = e.layerX;
        starty = e.layerY;
    });
    overlay.addEventListener('mousemove', function(e) {
        if(mouse_moving) {
            context.beginPath();
            context.moveTo(startx, starty);
            context.lineTo(e.layerX, e.layerY);
            context.stroke();

            lines.push([[startx, starty], [e.layerX, e.layerY]]);

            startx = e.layerX;
            starty = e.layerY;
        } else {
            // draw sample line
            overlay_ctx.clearRect(0, 0, overlay.width, overlay.height);
            overlay_ctx.beginPath()
            overlay_ctx.moveTo(startx, starty)
            overlay_ctx.lineTo(e.layerX, e.layerY)
            overlay_ctx.stroke()

        }
    })
    overlay.addEventListener('mouseup', function(e) {
        mouse_moving = false
    })
    overlay.addEventListener('mouseout', function(e) {
        overlay_ctx.clearRect(0, 0, overlay.width, overlay.height);

    })



    document.getElementById("print").onclick = () => {
        // const text = lines.map(x => process_line(x)).join("\n");
        // console.log(text);
        // copyTextToClipboard(text);
        fetch("print", {
            method: "POST",
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify(lines.flatMap(id).map(process_point))
        })
    };
    document.getElementById("clear").onclick = () => {
        lines = []
        clear();
    }
}
const id = x => x;

function process_point(point) { // expects [x, y]
    // first we normalize the points
    return [Math.floor((point[0] - document.getElementById('canvas').width/2)/5),
            Math.floor((point[1] - document.getElementById('canvas').height/2)/5)]
}

function send_instr(instr) {
    fetch("instr", {
        method: "POST",
        headers: {
            'Content-Type': 'text/plain'
        },
        body: instr
    })
}