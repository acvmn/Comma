function createBanner() {
    const banner = document.createElement("div");
    banner.style.position = "fixed";
    banner.style.width = "100%";
    banner.style.maxWidth = "1000px";
    banner.style.bottom = "80px";
    banner.style.backgroundColor = "var(--card-color)";
    banner.style.padding = "20px";
    banner.style.borderRadius = "20px";
    banner.style.boxShadow = "0px 5px 35px 7px rgba(0, 0, 0, 0.15)";
    banner.style.color = "var(--gray-color)";

    const message = document.createElement("p");
    message.innerHTML = 'Продолжая пользоваться сайтом, вы соглашаетесь с <a href="/agreement/">пользовательским соглашением</a>, <a href="/moderation/">правилами модерации</a> и <a href="/policy/">политикой конфиденциальности</a>. В случае отказа уходите с сайта. Все документы находятся в вкладке "Аккаунт".';

    const acceptButton = document.createElement("button");
    acceptButton.style.backgroundColor = "var(--first-red-color)";
    acceptButton.style.color = "var(--font-color)";
    acceptButton.style.padding = "5px";
    acceptButton.style.border = "none";
    acceptButton.style.borderRadius = "5px";
    acceptButton.style.margin = "10px 5px 0px 0px";
    acceptButton.style.cursor = "pointer";
    acceptButton.textContent = "Согласен";
    acceptButton.addEventListener("click", function () {
        localStorage.setItem("agreement", "true");
        banner.remove();
    });

    banner.appendChild(message);
    banner.appendChild(acceptButton);

    document.body.appendChild(banner);
}

window.onload = function () {
    if (localStorage.getItem("agreement") == null) {
        createBanner();
    }
};