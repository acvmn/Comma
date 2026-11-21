let wsUrl = `wss://${location.hostname}:443/room/`;
let ws, pc, localStream, localScreenStream = null, role = null;
let isCallActive = false;

var notifications = 0;
const not_audio = new Audio("notification.mp3");
var peer_audio;

var developer = false;

const config = {
    iceServers: [
        { urls: "stun:stun.l.google.com:19302" },
        {
            urls: "turn:132.243.230.30:3478",
            username: "myuser",
            credential: "mypassword123"
        }
    ]
};

const main = document.getElementById("main");
const title = document.getElementById("title");
const status = document.getElementById("status");
const createBtn = document.getElementById("createBtn");
const divs = document.querySelectorAll(".div");
const room = document.getElementById("room");
const connectBtn = document.getElementById("connectBtn");
const remoteAudioDiv = document.getElementById("remoteAudio");
const header = document.getElementById("top");
const screenContainer = document.getElementById("screen-container");
const demoInfo = document.getElementById("demo-info");
const localScreenPreview = document.getElementById("localScreenPreview");
const remoteScreen = document.getElementById("remoteScreen");
const bottom = document.getElementById("bottom");
const shareScreenBtn = document.getElementById("shareScreenBtn");
const chatBtn = document.getElementById("chatBtn");
const messages = document.getElementById("messages");
const child = document.getElementById("child");
const input = document.getElementById("input");

function notification(text) {
    const existingToast = document.querySelector(".toast-notification");
    if (existingToast) {
        existingToast.remove();
    }

    const toast = document.createElement("div");
    toast.className = "toast-notification";

    toast.innerHTML = text;

    document.body.appendChild(toast);

    setTimeout(function () {
        if (toast && toast.parentNode) {
            toast.remove();
        }
    }, 5000);
}

function limitText(limitField, limitNum) {
    if (limitField.value.length > limitNum) {
        limitField.value = limitField.value.substring(0, limitNum);
    }
}

function create() {
    const login = localStorage.getItem("login");
    const password = localStorage.getItem("password");

    const request = {
        Command: "Create"
    };
    const jsonString = JSON.stringify(request);

    const xhr = new XMLHttpRequest();
    xhr.open("POST", "/api/");
    xhr.setRequestHeader("Login", login);
    xhr.setRequestHeader("Password", password);
    xhr.send(jsonString);
    xhr.onload = function () {
        const data = JSON.parse(xhr.responseText);

        developer = (data.Developer == "true") ? true : false;

        if (data.Room != 0) {
            wsUrl += data.Room;
            start();
        }
        else {
            notification('<i class="fas fa-check-circle"></i><span id="span">Все комнаты заняты.</span><i class="fas fa-image" style="opacity: 0.8;"></i>')
        }
    };
}

function connect() {
    const login = localStorage.getItem("login");
    const password = localStorage.getItem("password");

    const request = {
        Connect: room.value
    };
    const jsonString = JSON.stringify(request);

    const xhr = new XMLHttpRequest();
    xhr.open("POST", "/api/");
    xhr.setRequestHeader("Login", login);
    xhr.setRequestHeader("Password", password);
    xhr.send(jsonString);
    xhr.onload = function () {
        const data = JSON.parse(xhr.responseText);

        developer = (data.Developer == "true") ? true : false;

        if (data.Status == "Found") {
            wsUrl += room.value;
            start();
        }
        else {
            notification('<i class="fas fa-check-circle"></i><span id="span">Комната не найдена</span><i class="fas fa-image" style="opacity: 0.8;"></i>')
        }
    }
}

async function start() {
    try {
        localStream = await navigator.mediaDevices.getUserMedia({ audio: true, video: false });
        status.innerText = "Микрофон включен. Подключаемся...";
        status.style.display = "initial";
        createBtn.style.display = "none";
        divs.forEach(element => {
            element.style.display = "none";
        });
        room.style.display = "none";
        connectBtn.style.display = "none";
        connectWebSocket();
    } catch (e) {
        notification('<i class="fas fa-check-circle"></i><span id="span">Микрофон выключен.</span><i class="fas fa-image" style="opacity: 0.8;"></i>')
        localStream = new MediaStream();
        status.innerText = "Микрофон выключен. Подключаемся...";
        status.style.display = "initial";
        createBtn.style.display = "none";
        divs.forEach(element => {
            element.style.display = "none";
        });
        room.style.display = "none";
        connectBtn.style.display = "none";
        connectWebSocket();
    }
}

async function copyTextUsingExecCommand(text) {
    await navigator.clipboard.writeText(text);
    return false;
}

function connectWebSocket() {
    ws = new WebSocket(wsUrl);
    ws.onopen = () => status.innerText = "Ждем собеседника комнаты " + wsUrl.split("/")[4] + "...";
    status.style.cursor = "pointer";
    status.onclick = () => {
        copyTextUsingExecCommand(wsUrl.split("/")[4])
        notification('<i class="fas fa-check-circle"></i><span id="span">Комната скопирована.</span><i class="fas fa-image" style="opacity: 0.8;"></i>')
    };
    ws.onmessage = async (e) => {
        const msg = JSON.parse(e.data);
        if (msg.type === "role") {
            role = msg.role;
            if (role === "offerer") {
                status.innerText = "Вы инициатор. Создаем соединение...";
                await createPeerConnection();
                localStream.getTracks().forEach(t => pc.addTrack(t, localStream));
                const offer = await pc.createOffer();
                await pc.setLocalDescription(offer);
                ws.send(JSON.stringify({ type: "offer", sdp: pc.localDescription }));
                enableScreenShare();
            } else {
                status.innerText = "Вы отвечающий. Ожидание входящего звонка...";
            }
        } else if (msg.type === "offer") {
            if (!isCallActive) {
                await createPeerConnection();
                localStream.getTracks().forEach(t => pc.addTrack(t, localStream));
                await pc.setRemoteDescription(new RTCSessionDescription(msg.sdp));
                const answer = await pc.createAnswer();
                await pc.setLocalDescription(answer);
                ws.send(JSON.stringify({ type: "answer", sdp: pc.localDescription }));
                isCallActive = true;
                status.style.display = "none";
                enableScreenShare();
            } else {
                await pc.setRemoteDescription(new RTCSessionDescription(msg.sdp));
                const answer = await pc.createAnswer();
                await pc.setLocalDescription(answer);
                ws.send(JSON.stringify({ type: "answer", sdp: pc.localDescription }));
            }
        } else if (msg.type === "answer") {
            if (!isCallActive) {
                await pc.setRemoteDescription(new RTCSessionDescription(msg.sdp));
                isCallActive = true;
                status.style.display = "none";
            } else {
                await pc.setRemoteDescription(new RTCSessionDescription(msg.sdp));
            }
        } else if (msg.type === "ice_candidate") {
            try { await pc.addIceCandidate(new RTCIceCandidate(msg.candidate)); } catch (e) { }
        } else if (msg.type === "message") {
            const text = document.createElement('p');
            text.textContent = msg.message;
            child.appendChild(text);
            if (messages.style.display == "none") {
                notifications++;
                chatBtn.innerHTML = '<i class="fas fa-message"></i>Чат (' + notifications + ')';
            }
            not_audio.pause();
            not_audio.currentTime = 0;
            not_audio.play();
        } else if (msg.type === "demo" && msg.message === "off") {
            remoteScreen.srcObject = null;
            remoteScreen.style.display = "none";
            if (localScreenStream == null && remoteScreen.srcObject == null) {
                demoInfo.style.display = "block";
            }
        } else if (msg.type === "peer_disconnected") {
            location.reload();
        } else if (msg.type === "status" && msg.status === "occupied") {
            location.reload();
        } else if (msg.type === "status" && msg.status === "error") {
            location.reload();
        }
    };
}

function formatTime(seconds) {
    const hors = Math.floor(seconds / 60 / 60).toString().padStart(2, '0');
    const mins = Math.floor((seconds / 60) % 60).toString().padStart(2, '0');
    const secs = (seconds % 60).toString().padStart(2, '0');
    return `${hors}:${mins}:${secs}`;
}

async function createPeerConnection() {
    pc = new RTCPeerConnection(config);
    pc.onicecandidate = e => {
        if (e.candidate) ws.send(JSON.stringify({ type: "ice_candidate", candidate: e.candidate }));
    };
    pc.ontrack = e => {
        if (e.track.kind === "audio") {
            remoteAudioDiv.innerHTML = "";
            peer_audio = document.createElement("audio");
            peer_audio.autoplay = true;
            peer_audio.display = false;
            peer_audio.srcObject = e.streams[0];
            remoteAudioDiv.appendChild(audio);
        } else if (e.track.kind === "video") {
            demoInfo.style.display = "none";
            remoteScreen.srcObject = e.streams[0];
            remoteScreen.style.display = "inline-block";
        }
    };

    notification('<i class="fas fa-check-circle"></i><span id="span">Вы вошли в звонок.</span><i class="fas fa-image" style="opacity: 0.8;"></i>')
    main.style.display = "none";
    header.style.display = "flex";
    screenContainer.style.display = "flex";
    bottom.style.display = "flex";
    title.innerHTML = 'Прошло: <span id="timer">00:00:00</span>';
    let totalSeconds = 0;
    const element = document.getElementById('timer');
    setInterval(() => {
        totalSeconds++;
        element.textContent = formatTime(totalSeconds);
    }, 1000);

    const style = document.createElement('style');
    style.textContent = `
                @media (max-width: 400px) {
                    .screen-container {
                        display: flex;
                        flex-direction: column;
                    }

                    .screen-video {
                        width: calc(100vw - 30px);
                    }
                }
            `;
    document.head.appendChild(style);
}

function enableScreenShare() {
    shareScreenBtn.style.display = "inline-flex";
    shareScreenBtn.disabled = false;

    chatBtn.style.display = "inline-flex";
    chatBtn.disabled = false;
}

shareScreenBtn.onclick = async () => {
    if (localScreenStream) {
        stopScreenShare();
        return;
    }

    try {
        localScreenStream = await navigator.mediaDevices.getDisplayMedia({ video: true, audio: true });
        const screenTrack = localScreenStream.getVideoTracks()[0];
        if (!screenTrack) throw new Error("No video track");

        demoInfo.style.display = "none";

        localScreenPreview.srcObject = localScreenStream;
        localScreenPreview.style.display = "inline-block";

        localScreenStream.getTracks().forEach(track => {
            pc.addTrack(track, localScreenStream);
        });

        const offer = await pc.createOffer();
        await pc.setLocalDescription(offer);
        ws.send(JSON.stringify({ type: "offer", sdp: pc.localDescription }));

        shareScreenBtn.innerHTML = '<i class="fas fa-stop-circle"></i>Остановить';

        screenTrack.onended = () => {
            stopScreenShare();
        };
    } catch (err) {
        notification('<i class="fas fa-check-circle"></i><span id="span">Не удалось начать демонстрацию.</span><i class="fas fa-image" style="opacity: 0.8;"></i>')
    }
};

chatBtn.onclick = async () => {
    if (messages.style.display == "none") {
        messages.style.display = "inline-block";
        input.style.display = "inline-block";
        notifications = 0;
        chatBtn.innerHTML = '<i class="fas fa-message"></i>Чат';
    }
    else {
        messages.style.display = "none";
        input.style.display = "none";
    }
};

input.addEventListener("keyup", (event) => {
    if (!developer) {
        input.value = "";
        notification('<i class="fas fa-check-circle"></i><span id="span">В чат могут писать только разработчики!</span><i class="fas fa-image" style="opacity: 0.8;"></i>')
    }
    if (event.key === "Enter" && input.value != "" && !event.shiftKey && !(/^\s*$/.test(input.value))) {
        const text = document.createElement("p");
        text.textContent = input.value;
        child.appendChild(text);

        const message = {
            type: "message",
            message: input.value,
            password: localStorage.getItem("password")
        };
        const jsonString = JSON.stringify(message);
        ws.send(jsonString);

        input.value = "";
    }
});

async function stopScreenShare() {
    if (localScreenStream) {
        localScreenStream.getTracks().forEach(track => {
            track.stop();
        });
        localScreenStream = null;

        localScreenPreview.srcObject = null;
        localScreenPreview.style.display = "none"

        if (localScreenStream == null && remoteScreen.srcObject == null) {
            demoInfo.style.display = "block";
        }

        ws.send(JSON.stringify({ type: "demo", message: "off" }));
    }

    shareScreenBtn.innerHTML = '<i class="fas fa-desktop"></i>Демонстрация';
    shareScreenBtn.style.background = "#dc2626";
}

createBtn.onclick = () => create();
connectBtn.onclick = () => connect();