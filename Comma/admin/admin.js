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

function rebootServer() {
    const login = localStorage.getItem("login");
    const password = localStorage.getItem("password");

    const request = {
        Command: "Stop..."
    };
    const jsonString = JSON.stringify(request);

    var xhr = new XMLHttpRequest();
    xhr.open("POST", "/admin/");
    xhr.setRequestHeader("Login", login);
    xhr.setRequestHeader("Password", password);
    xhr.send(jsonString);
    xhr.onload = function () {
        var response = xhr.responseText;
        if (response == "") {
            notification('<i class="fas fa-check-circle"></i><span id="span">Пустой ответ. Скорее всего вы не админ.</span><i class="fas fa-image" style="opacity: 0.8;"></i>');
        }
        else {
            const data = JSON.parse(response);
            if (data.Status == "Stop") {
                notification('<i class="fas fa-check-circle"></i><span id="span">Сервер был остановлен.</span><i class="fas fa-image" style="opacity: 0.8;"></i>');
            }
        }
    }
}

document.getElementById("rebootBtn").addEventListener("click", function () {
    rebootServer();
});