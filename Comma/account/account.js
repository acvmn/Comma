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
function reg() {
    var login = document.getElementById("login").value;
    var password = document.getElementById("password").value;

    if (login != "") {
        if (password != "") {
            if (login.length <= 8) {
                if (password.length <= 8) {
                    if (login.toLowerCase() == login) {
                        if (password.toLowerCase() == password) {
                            const request = {
                                Command: "Reg",
                                Login: login,
                                Password: password
                            };
                            const jsonString = JSON.stringify(request);

                            var xhr = new XMLHttpRequest();
                            xhr.open("POST", "/api/");
                            xhr.send(jsonString);
                            xhr.onload = function () {
                                var response = xhr.responseText;
                                if (response != "") {
                                    const data = JSON.parse(response);
                                    if (data.Status == "Busy") {
                                        notification('<i class="fas fa-check-circle"></i><span id="span">Этот логин уже используется.</span><i class="fas fa-image" style="opacity: 0.8;"></i>')
                                        document.getElementById("login").style.backgroundColor = "#ffc3c3";
                                        document.getElementById("password").style.backgroundColor = "#d7ffda";
                                    }
                                    else if (data.Status == "Many") {
                                        notification('<i class="fas fa-check-circle"></i><span id="span">У вас слишком много аккаунтов.</span><i class="fas fa-image" style="opacity: 0.8;"></i>')
                                        document.getElementById("login").style.backgroundColor = "#ffc3c3";
                                        document.getElementById("password").style.backgroundColor = "#ffc3c3";
                                    }
                                    else if (data.Status == "Wrong") {
                                        notification('<i class="fas fa-check-circle"></i><span id="span">Неправильный логин.</span><i class="fas fa-image" style="opacity: 0.8;"></i>')
                                        document.getElementById("login").style.backgroundColor = "#ffc3c3";
                                        document.getElementById("password").style.backgroundColor = "#d7ffda";
                                    }
                                }
                                else if (response == "") {
                                    notification('<i class="fas fa-check-circle"></i><span id="span">Регистрация успешна!</span><i class="fas fa-image" style="opacity: 0.8;"></i>')
                                    localStorage.setItem("ID", login);
                                    localStorage.setItem("login", login);
                                    localStorage.setItem("password", password);
                                    document.getElementById("login").style.backgroundColor = "#d7ffda";
                                    document.getElementById("password").style.backgroundColor = "#d7ffda";
                                }
                            }
                        }
                        else {
                            notification('<i class="fas fa-check-circle"></i><span id="span">Разрешены пароли только в строчном регистре.</span><i class="fas fa-image" style="opacity: 0.8;"></i>')
                            document.getElementById("login").style.backgroundColor = "#d7ffda";
                            document.getElementById("password").style.backgroundColor = "#ffc3c3";
                        }
                    }
                    else {
                        notification('<i class="fas fa-check-circle"></i><span id="span">Разрешены логины только в строчном регистре.</span><i class="fas fa-image" style="opacity: 0.8;"></i>')
                        document.getElementById("login").style.backgroundColor = "#ffc3c3";
                        document.getElementById("password").style.backgroundColor = "#d7ffda";
                    }
                }
                else {
                    notification('<i class="fas fa-check-circle"></i><span id="span">Пароль слишком длинный.</span><i class="fas fa-image" style="opacity: 0.8;"></i>')
                    document.getElementById("login").style.backgroundColor = "#d7ffda";
                    document.getElementById("password").style.backgroundColor = "#ffc3c3";
                }
            }
            else {
                notification('<i class="fas fa-check-circle"></i><span id="span">Логин слишком длинный.</span><i class="fas fa-image" style="opacity: 0.8;"></i>')
                document.getElementById("login").style.backgroundColor = "#ffc3c3";
                document.getElementById("password").style.backgroundColor = "#d7ffda";
            }
        }
        else {
            notification('<i class="fas fa-check-circle"></i><span id="span">Пароль не должен быть пустой.</span><i class="fas fa-image" style="opacity: 0.8;"></i>')
            document.getElementById("login").style.backgroundColor = "#d7ffda";
            document.getElementById("password").style.backgroundColor = "#ffc3c3";
        }
    }
    else {
        notification('<i class="fas fa-check-circle"></i><span id="span">Логин не должен быть пустой.</span><i class="fas fa-image" style="opacity: 0.8;"></i>')
        document.getElementById("login").style.backgroundColor = "#ffc3c3";
        document.getElementById("password").style.backgroundColor = "#d7ffda";
    }
}

function auth() {
    var login = document.getElementById("login").value;
    var password = document.getElementById("password").value;

    if (login != "") {
        if (password != "") {
            const request = {
                Command: "Auth",
                Login: login,
                Password: password
            };
            const jsonString = JSON.stringify(request);

            var xhr = new XMLHttpRequest();
            xhr.open("POST", "/api/");
            xhr.send(jsonString);
            xhr.onload = function () {
                var response = xhr.responseText;
                if (response != "") {
                    const data = JSON.parse(response);
                    if (data.Status == "Wrong") {
                        notification('<i class="fas fa-check-circle"></i><span id="span">Неверный логин или пароль.</span><i class="fas fa-image" style="opacity: 0.8;"></i>')
                        document.getElementById("login").style.backgroundColor = "#ffc3c3";
                        document.getElementById("password").style.backgroundColor = "#ffc3c3";
                    }
                }
                else if (response == "") {
                    notification('<i class="fas fa-check-circle"></i><span id="span">Вход успешен!</span><i class="fas fa-image" style="opacity: 0.8;"></i>')
                    localStorage.setItem("login", login);
                    localStorage.setItem("password", password);
                    document.getElementById("login").style.backgroundColor = "#d7ffda";
                    document.getElementById("password").style.backgroundColor = "#d7ffda";
                }
            }
        }
        else {
            notification('<i class="fas fa-check-circle"></i><span id="span">Пароль не должен быть пустой.</span><i class="fas fa-image" style="opacity: 0.8;"></i>')
            document.getElementById("login").style.backgroundColor = "#d7ffda";
            document.getElementById("password").style.backgroundColor = "#ffc3c3";
        }
    }
    else {
        notification('<i class="fas fa-check-circle"></i><span id="span">Логин не должен быть пустой.</span><i class="fas fa-image" style="opacity: 0.8;"></i>')
        document.getElementById("login").style.backgroundColor = "#ffc3c3";
        document.getElementById("password").style.backgroundColor = "#d7ffda";
    }
}

function out() {
    notification('<i class="fas fa-check-circle"></i><span id="span">Вы успешно вышли!</span><i class="fas fa-image" style="opacity: 0.8;"></i>')
    localStorage.removeItem("login");
    localStorage.removeItem("password");
    document.getElementById("login").style.backgroundColor = "#d7ffda";
    document.getElementById("password").style.backgroundColor = "#d7ffda";
}

document.getElementById("login").addEventListener("input", function () {
    this.value = this.value.toLowerCase();
});

document.getElementById("password").addEventListener("input", function () {
    this.value = this.value.toLowerCase();
});