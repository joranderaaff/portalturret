var endpoint = window.location.hostname == "127.0.0.1" ? "192.168.2.124" : window.location.hostname;
var accelerometerPre;
var px, py, pz;
var currentDataIndex = 0;
var currentState = -1;
var isPlayingAudio = false;
var wasOpen = undefined;
var isOpen = undefined;
var stateNames = [
    "Idle",
    "Activated",
    "Searching",
    "Engaging",
    "Target Lost",
    "Picked Up",
    "Shutdown",
    "Rebooting"
]
var cvs = document.getElementById('accel-canvas');
var ctx = cvs.getContext('2d');
var imageData = ctx.getImageData(0, 0, cvs.width, cvs.height);

var sentences = [];
var currentSentence = 0;
var wasDetectingMovement = false;
var manualOverride = false;
var stateButtons = document.getElementsByClassName("state-button");
var logContainer;
function* routine() {
    var pre;
    logContainer = document.getElementById('log');
    pre = document.createElement('pre');
    logContainer.appendChild(pre);

    while (true) {
        if (currentSentence == sentences.length) {
            yield;
        } else {
            yield* typedSentenceRoutine(pre, sentences[currentSentence].text, sentences[currentSentence].speed);
            currentSentence++;
        }
    }
}

function* typedSentenceRoutine(element, text, speed) {
    var baseText = element.innerHTML;
    var newText = "";
    var letter = 0;
    while (letter < text.length) {
        var lettersPerTick = speed >= 1 ? speed : 1;
        var ticksPerLetter = speed < 1 ? 1 / speed : 1;
        newText += text.substring(letter, letter + lettersPerTick).replace(/[ ]/g, "&nbsp;");
        element.innerHTML = baseText + newText;
        logContainer.scrollTop = logContainer.scrollHeight;
        letter += lettersPerTick;
        for (var i = 0; i < ticksPerLetter; i++) {
            yield;
        }
    }
}

function* delayTicks(ticks) {
    var i = 0;
    while (i < ticks) {
        i++;
        yield;
    }
}

var d = new Date();
sentences.push({
    text: d.getDate() + "-" + (d.getMonth() + 1) + "-" + d.getFullYear() + " " + d.getHours() + ":" + d.getMinutes() + "\n",
    speed: 1
});
sentences.push({ text: "Aperture Science Sentry\n", speed: 1 });
sentences.push({ text: "Unit [FUAX-3722-1628]\n", speed: 1 });
sentences.push({ text: "Retreiving Status", speed: 1 });
sentences.push({ text: "........\n", speed: 0.1 });
sentences.push({ text: "Status: [Online]\n", speed: 1 });

var r = routine();
setInterval(() => {
    r.next();
}, 16);

function setOpen(open) {
    var xhr = new XMLHttpRequest();
    var fd = new FormData();
    fd.append("open", open ? 1 : 0);
    xhr.open("POST", `http://${endpoint}/set_open`, true);
    xhr.send(fd);
}

function setState(state) {
    var xhr = new XMLHttpRequest();
    var fd = new FormData();
    fd.append("state", state);
    xhr.open("POST", `http://${endpoint}/set_state`, true);
    xhr.send(fd);
}

console.log(`ws://${endpoint}:81/`);
var connection = new WebSocket(`ws://${endpoint}:81/`);
connection.binaryType = 'arraybuffer';

connection.onopen = () => {
    console.log("connected");
};

connection.onerror = (error) => {
    console.log('WebSocket Error ', error);
};

function twosComplement(x) {
    if (x & 0x8000) {
        x--;
        x = (~x) & 0xFFFF;
        x = -x;
    }
    return x;
}

connection.onmessage = (e) => {
    const view = new Uint8Array(e.data);
    var x = (view[0] << 8) | view[1];
    var y = (view[2] << 8) | view[3];
    var z = (view[4] << 8) | view[5];

    x = twosComplement(x);
    y = twosComplement(y);
    z = twosComplement(z);

    x = x * 0.004 * 9.80665;
    y = y * 0.004 * 9.80665;
    z = z * 0.004 * 9.80665;

    draw(currentDataIndex, cvs.height / 2 - Math.round(px * 2), cvs.height / 2 - Math.round(x * 2), '#a88438');
    draw(currentDataIndex, cvs.height / 2 - Math.round(py * 2), cvs.height / 2 - Math.round(y * 2), '#a88438');
    draw(currentDataIndex, cvs.height / 2 - Math.round(pz * 2), cvs.height / 2 - Math.round(z * 2), '#a88438');

    px = x;
    py = y;
    pz = z;

    currentDataIndex++;
    if (currentDataIndex > cvs.width / 2) {
        currentDataIndex = 0;
        ctx.clearRect(0, 0, cvs.width, cvs.height);
    }

    var detectingMovement = view[7] == 1;
    if (!wasDetectingMovement && detectingMovement) {
        d = new Date();
        var h = d.getHours().toString().padStart(2, "0");
        var m = d.getMinutes().toString().padStart(2, "0");
        var s = d.getSeconds().toString().padStart(2, "0");
        sentences.push({ text: `${getTimeStamp()} Movement detected!\n`, speed: 3 });
    }
    wasDetectingMovement = detectingMovement;

    isOpen = view[6] !== 1;

    if(wasOpen === undefined || isOpen !== wasOpen) {
        updateManualButtons(isOpen);
    }
    wasOpen = isOpen;

    var m = view[7] == 1 ? "There you are!" : "Where Are you?";
    var a = (view[8] << 8) | view[9];
    var state = parseInt(view[10]);
    var playingAudio = parseInt(view[11]);
    if(playingAudio !== isPlayingAudio) {
        isPlayingAudio = playingAudio;
        console.log("AUDIO PLAY: " + isPlayingAudio);
    }

    if (state != currentState) {
        for (var i = 0; i < stateButtons.length; i++) {
            var b = stateButtons[i];
            let buttonState = parseInt(b.getAttribute('data-state'));
            if (buttonState == state) {
                b.classList.add('selected');
            } else {
                b.classList.remove('selected');
            }
        };
        var stateName = stateNames[state];
        if(state === 6) {
            document.body.classList.add('error');
            sentences.push({ text: `[[[ CRITICAL ERROR ]]]\n`, speed: 3 });
        } else if(state === 8) {
            document.body.classList.remove('error');
            sentences.push({ text: `Rebooting`, speed: 3 });
            sentences.push({ text: `......\n`, speed: 0.05 });
        } else {
            sentences.push({ text: `${getTimeStamp()} ${stateName}\n`, speed: 3 });
        }
        currentState = state;
    }
};

function getTimeStamp() {
    d = new Date();
    var h = d.getHours().toString().padStart(2, "0");
    var m = d.getMinutes().toString().padStart(2, "0");
    var s = d.getSeconds().toString().padStart(2, "0");
    return `${h}:${m}:${s}`;
}

function setPixel(x, y, r, g, b) {
    var i = (y * cvs.width + x) * 4;
    imageData.data[i + 0] = r;
    imageData.data[i + 1] = g;
    imageData.data[i + 2] = b;
    imageData.data[i + 3] = 255;
}

function draw(x, y1, y2, c) {
    ctx.beginPath();
    ctx.strokeStyle = c;
    ctx.moveTo(x * 2, y1);
    ctx.lineTo(x * 2 + 2, y2);
    ctx.stroke();
}

connection.onclose = () => {
    console.log('WebSocket connection closed');
};

window.onbeforeunload = function () {
    connection.onclose = function () { }; // disable onclose handler first
    connection.close();
};

document.getElementById('button-override').addEventListener('click', () => {
    toggleControl(false);
});

document.getElementById('button-automatic').addEventListener('click', () => {
    toggleControl(true);
});

var currentAngle = 0;
var sentAngle = 0;
var movementData = new Uint8Array(2);

document.getElementById('button-fire').addEventListener('click', ()=>{
    movementData[0] = 1;
    connection.send(movementData);
});

document.getElementById('button-left').addEventListener('mousedown', ()=>{
    movementData[0] = 2;
    movementData[1] = 1;
    console.log(1);
    connection.send(movementData);
});

document.getElementById('button-right').addEventListener('mousedown', ()=>{
    movementData[0] = 2;
    movementData[1] = 2;
    console.log(2);
    connection.send(movementData);
});

document.getElementById('button-left').addEventListener('mouseup', ()=>{
    movementData[0] = 2;
    movementData[1] = 0;
    console.log(0);
    connection.send(movementData);
});

document.getElementById('button-right').addEventListener('mouseup', ()=>{
    movementData[0] = 2;
    movementData[1] = 0;
    console.log(0);
    connection.send(movementData);
});

document.getElementById('button-wings').addEventListener('mousedown', () => {
    if(isOpen) {
        setOpen(false);
    } else {
        setOpen(true);
    }
});

updateManualButtons = function(isOpen) {
    document.getElementById('button-wings').setAttribute('value', isOpen ?  "Close" : "Open");
    if(isOpen) {
        document.getElementById('button-fire').removeAttribute("disabled");
        document.getElementById('button-left').removeAttribute("disabled");
        document.getElementById('button-right').removeAttribute("disabled");
    } else {
        document.getElementById('button-fire').setAttribute("disabled", 1);
        document.getElementById('button-left').setAttribute("disabled", 1);
        document.getElementById('button-right').setAttribute("disabled", 1);
    }
}

toggleControl = function (automatic) {
    var fd = new FormData();
    var xhr = new XMLHttpRequest();

    if (!automatic) {
        fd.append("mode", 1);
        sentences.push({ text: "[Manual Mode] Activated.\n", speed: 1 });
        document.body.classList.add('manual');
    } else {
        fd.append("mode", 0);
        sentences.push({ text: "[Manual Mode] Deactivated.\n", speed: 1 });
        document.body.classList.remove('manual');
    }

    xhr.open("POST", `http://${endpoint}/set_mode`, true);
    xhr.send(fd);
}

for (var i = 0; i < stateButtons.length; i++) {
    let state = stateButtons[i].getAttribute('data-state');
    stateButtons[i].addEventListener('mousedown', () => {
        setState(state);
    });
}

updateManualButtons(isOpen);