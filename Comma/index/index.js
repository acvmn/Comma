var ID = "Гость";
var array;
const path = "https://" + window.location.hostname + "/";
const feedContainer = document.getElementById("feed_container");
const likesArray = [];
var cops = [];
function formatTime(seconds) {
    const diffSec = seconds;
    const diffMin = Math.floor(seconds / 60);
    const diffHour = Math.floor(diffMin / 60);

    if (diffSec < 60) return "До следующей публикации поста осталось подождать " + diffSec + " " + declension(diffMin, "секунду.", "секунды.", "секунд.");
    if (diffMin < 60) return "До следующей публикации поста осталось подождать " + diffMin + " " + declension(diffMin, "минуту.", "минуты.", "минут.");
    if (diffHour) return "До следующей публикации поста осталось подождать " + diffHour + " " + declension(diffHour, "час.", "часа.", "часов.");
}

async function size(attachment) {
    const file = new Response(attachment);
    const blob = await file.blob();
    if (blob.size > (1024 * 1024 * 16) || (attachment.get("userfile").name.split('.').pop().toLowerCase() == "txt") && blob.size > (1024 * 16)) {
        return false;
    }
    return true;
}

async function new_file_post(e, file) {
    const login = localStorage.getItem("login");
    const password = localStorage.getItem("password");

    notification('<i class="fas fa-check-circle"></i><span id="span">Публикация...</span><i class="fas fa-image" style="opacity: 0.8;"></i>');

    e.preventDefault();
    e.stopPropagation();
    const userfile1 = file;
    const attachment = new FormData();
    attachment.append("userfile", userfile1, fileName(userfile1.name));

    if (await size(attachment)) {
        const xhr = new XMLHttpRequest();
        xhr.open("POST", "/upload");
        if (login && password) {
            xhr.setRequestHeader("Login", login);
            xhr.setRequestHeader("Password", password);
        }
        xhr.upload.addEventListener("progress", function (event) {
            const percentComplete = (event.loaded / event.total) * 100;
            document.getElementById("progress").value = percentComplete;
        });
        xhr.send(attachment);

        xhr.onload = function () {
            if (xhr.status == 429) {
                const data = JSON.parse(xhr.responseText);
                notification('<i class="fas fa-check-circle"></i><span id="span">' + formatTime(data.Time) + '</span><i class="fas fa-image" style="opacity: 0.8;"></i>');
            }
            else if (xhr.status == 200) {
                const ext = fileName(userfile1.name).split(".").pop();
                if (ext !== "png" && ext !== "jpg" && ext !== "jpeg" && ext !== "gif" && ext !== "ico" && ext !== "mp4") {
                    const userfile2 = textToFile(path + "upload/" + ID + "/" + fileName(userfile1.name));
                    const attachment2 = new FormData();
                    attachment2.append("userfile", userfile2.blob, userfile2.fileName);

                    const xhr2 = new XMLHttpRequest();
                    xhr2.open("POST", "/upload");
                    if (login && password) {
                        xhr2.setRequestHeader("Login", login);
                        xhr2.setRequestHeader("Password", password);
                    }
                    xhr2.send(attachment2);
                }

                location.reload();
            }
        };
        xhr.onerror = function () {
            notification('<i class="fas fa-check-circle"></i><span id="span">Не удалось загрузить файл. Попробуйте еще раз.</span><i class="fas fa-image" style="opacity: 0.8;"></i>');
            document.getElementById("progress").value = 0;
            document.getElementById("file_input").files[0] = null;
        };
    } else {
        notification('<i class="fas fa-check-circle"></i><span>Файл слишком большой!</span><i class="fas fa-image" style="opacity: 0.8;"></i>');
        document.getElementById("progress").value = 0;
        document.getElementById("file_input").files[0] = null;
    }
}

async function new_file_comment(file, id, post, comments, e) {
    const login = localStorage.getItem("login");
    const password = localStorage.getItem("password");

    notification('<i class="fas fa-check-circle"></i><span id="span">Публикация...</span><i class="fas fa-image" style="opacity: 0.8;"></i>');

    e.preventDefault();
    e.stopPropagation();
    const userfile1 = file;
    const attachment = new FormData();
    attachment.append("userfile", userfile1, fileName(userfile1.name));

    if (await size(attachment)) {
        const xhr = new XMLHttpRequest();
        xhr.open("POST", "/upload");
        if (login && password) {
            xhr.setRequestHeader("Login", login);
            xhr.setRequestHeader("Password", password);
        }
        xhr.setRequestHeader("Comment", post);
        xhr.upload.addEventListener("progress", function (event) {
            const percentComplete = (event.loaded / event.total) * 100;
            document.getElementById("progress_" + id).value = percentComplete;
        });
        xhr.send(attachment);

        xhr.onload = function () {
            if (xhr.status == 200) {
                const ext = fileName(userfile1.name).split(".").pop();
                if (ext !== "png" && ext !== "jpg" && ext !== "jpeg" && ext !== "gif" && ext !== "ico" && ext !== "mp4") {
                    const userfile2 = textToFile(path + "upload/" + ID + "/" + fileName(userfile1.name));
                    const attachment2 = new FormData();
                    attachment2.append("userfile", userfile2.blob, userfile2.fileName);

                    const xhr2 = new XMLHttpRequest();
                    xhr2.open("POST", "/upload");
                    xhr2.setRequestHeader("Comment", post);
                    xhr2.send(attachment2);
                }

                notification('<i class="fas fa-check-circle"></i><span id="span">Файл опубликован!</span><i class="fas fa-image" style="opacity: 0.8;"></i>');
                comments.remove();
                showComments(id, post.toString());
                document.querySelector('[data-post_count="' + id + '"]').textContent = Number(document.querySelector('[data-post_count="' + id + '"]').textContent) + 1;
            }
            else {
                notification('<i class="fas fa-check-circle"></i><span id="span">Не удалось загрузить файл. Попробуйте еще раз.</span><i class="fas fa-image" style="opacity: 0.8;"></i>');
                document.getElementById("progress_" + id).value = 0;
                document.getElementById("file_input_" + id).files[0] = null;
            }
        };
        xhr.onerror = function () {
            notification('<i class="fas fa-check-circle"></i><span id="span">Не удалось загрузить файл. Попробуйте еще раз.</span><i class="fas fa-image" style="opacity: 0.8;"></i>');
            document.getElementById("progress_" + id).value = 0;
            document.getElementById("file_input_" + id).files[0] = null;
        };
    } else {
        notification('<i class="fas fa-check-circle"></i><span>Файл слишком большой!</span><i class="fas fa-image" style="opacity: 0.8;"></i>');
        document.getElementById("progress_" + id).value = 0;
        document.getElementById("file_input_" + id).files[0] = null;
    }
}

function loadComments(id, arrayComments) {
    async function sequentialLoad() {
        const loadedImages = [];
        for (const url of arrayComments.Comments) {
            const user = url.split("\\")[1];
            if (user) {
                const response = await fetch(url, {
                    method: "GET",
                });

                const div = document.createElement("div");
                div.className = "comment-block";

                const userDiv = document.createElement("div");
                userDiv.className = "user";

                const divIcon = document.createElement("div");
                divIcon.className = "avatar-placeholder";

                const userIcon = document.createElement("i");
                userIcon.className = "far fa-user";
                divIcon.appendChild(userIcon);

                const tempElement = document.createElement("a");
                tempElement.className = "user-name";
                tempElement.textContent = user + ":";
                tempElement.href = "/profile/" + user;

                userDiv.appendChild(divIcon);
                userDiv.appendChild(tempElement);
                div.appendChild(userDiv);

                const extension = url.split(".").pop();
                if (extension === "png" || extension === "jpg" || extension === "jpeg" || extension === "gif" || extension === "ico") {
                    const center = document.createElement("center");
                    const blob = await response.blob();
                    const objectUrl = URL.createObjectURL(blob);
                    const img = document.createElement("img");
                    img.src = objectUrl;
                    img.preload = "auto";
                    img.className = "feed-image";
                    center.appendChild(img);
                    div.appendChild(center);
                    loadedImages.push(center);
                    img.onload = function () {
                        URL.revokeObjectURL(objectUrl);
                    };
                } else if (extension === "txt") {
                    const post = await response.text();

                    const postContent = document.createElement("div");
                    postContent.className = "post-content";

                    const postText = document.createElement("div");
                    postText.className = "post-text";
                    postText.textContent = post;

                    postContent.appendChild(postText);
                    div.appendChild(postContent);
                } else if (extension === "mp4") {
                    const center = document.createElement("center");
                    const blob = await response.blob();
                    const objectUrl = URL.createObjectURL(blob);
                    const video = document.createElement("video");
                    video.src = objectUrl;
                    video.controls = true;
                    video.loop = true;
                    video.className = "feed-image";
                    center.appendChild(video);
                    div.appendChild(center);
                    loadedImages.push(center);
                    video.onload = function () {
                        URL.revokeObjectURL(objectUrl);
                    };
                }

                const block = document.querySelector("#comments-" + id);
                block.insertAdjacentElement("beforeend", div);
            }
        }
        return loadedImages;
    }
    return sequentialLoad();
}

function showComments(id, post) {
    const comments = document.createElement("div");
    comments.id = "comments-" + id;
    const block = document.querySelector('[data-post-id="' + id + '"]');
    const toast = document.createElement("div");
    toast.className = "comment";
    toast.innerHTML = '<textarea class="text-input" id="text_' + id + '" placeholder="Ваш комментарий..."></textarea><progress class="comment-progress" id="progress_' + id +'" value="0" max="100"></progress><div class="upload"><input type="file" class="file_comment" id="file_input_' + id + '"><label for="file_input_' + id + '" class="file-input-label" id="upload_file_' + id + '"><svg xmlns="http://www.w3.org/2000/svg" width="32" height="32" fill="currentColor" class="bi bi-paperclip" viewBox="0 0 16 16"><path d="M4.5 3a2.5 2.5 0 0 1 5 0v9a1.5 1.5 0 0 1-3 0V5a.5.5 0 0 1 1 0v7a.5.5 0 0 0 1 0V3a1.5 1.5 0 1 0-3 0v9a2.5 2.5 0 0 0 5 0V5a.5.5 0 0 1 1 0v7a3.5 3.5 0 1 1-7 0z" /></svg></label><button class="text-input-label" onclick="new_text_comment(' + id + ', ' + post + ')"><i class="fas fa-cloud-upload-alt"></i>Добавить комментарий</button></div>';
    comments.appendChild(toast);
    block.appendChild(comments);

    document.getElementById("text_" + id).addEventListener("keyup", (event) => {
        if (event.key === "Enter" && document.getElementById("text_" + id).value != "" && !event.shiftKey && !(/^\s*$/.test(document.getElementById("text_" + id).value))) {
            new_text_comment(id, post);
        }
    });

    document.getElementById("file_input_" + id).addEventListener("change", function (e) {
        new_file_comment(document.getElementById("file_input_" + id).files[0], id, post, comments, e);
    });

    document.getElementById("upload_file_" + id).addEventListener("dragover", (event) => {
        event.preventDefault();
    });

    document.getElementById("upload_file_" + id).addEventListener("drop", function (e) {
        new_file_comment(e.dataTransfer.files[0], id, post, comments, e);
    });

    const request = {
        Comments: post
    };
    const jsonString = JSON.stringify(request);

    const xhrHistory = new XMLHttpRequest();
    xhrHistory.open("POST", "/api");
    xhrHistory.send(jsonString);
    xhrHistory.onload = function () {
        const start = xhrHistory.responseText;
        const data = JSON.parse(start);
        loadComments(id, data);
    };
}

function hideComments(id) {
    document.querySelector("#comments-" + id).remove();
}

function waitForScrollToBottom(offset) {
    return new Promise((resolve) => {
        const checkPosition = () => {
            const scrollPosition = window.innerHeight + window.scrollY;
            const pageHeight = document.documentElement.scrollHeight;
            if (scrollPosition >= pageHeight - offset) {
                window.removeEventListener("scroll", checkPosition);
                window.removeEventListener("resize", checkPosition);
                resolve();
            }
        };
        window.addEventListener("scroll", checkPosition);
        window.addEventListener("resize", checkPosition);
        checkPosition();
    });
}

function formatTimeAgo(dateString) {
    const now = new Date();
    const postDate = new Date(dateString);
    const diffMs = now - postDate;
    const diffSec = Math.floor(diffMs / 1000);
    const diffMin = Math.floor(diffSec / 60);
    const diffHour = Math.floor(diffMin / 60);
    const diffDay = Math.floor(diffHour / 24);

    if (diffSec < 60) return "только что";
    if (diffMin < 60) return diffMin + " " + declension(diffMin, "минуту", "минуты", "минут");
    if (diffHour < 24) return diffHour + " " + declension(diffHour, "час", "часа", "часов");
    if (diffDay < 7) return diffDay + " " + declension(diffDay, "день", "дня", "дней");

    let day = postDate.getDate().toString();
    let month = (postDate.getMonth() + 1).toString();

    if (day.length === 1) day = "0" + day;
    if (month.length === 1) month = "0" + month;

    return day + "." + month;
}

function declension(num, one, few, many) {
    num = Math.abs(num) % 100;
    if (num >= 11 && num <= 19) return many;
    const lastDigit = num % 10;
    if (lastDigit === 1) return one;
    if (lastDigit >= 2 && lastDigit <= 4) return few;
    return many;
}

async function copyTextUsingExecCommand(text) {
    await navigator.clipboard.writeText(text);
    return false;
}

async function download(step, url, div, ago, countLikes, countComments, bottom, img, postText, video, iconLikes, copyButton, extension) {
    const login = localStorage.getItem("login");
    const password = localStorage.getItem("password");

    const infoLikes = url.Liked + ": " + url.Likes;
    const infoViews = url.Views;
    const infoComments = url.Comments;
    const infoTime = url.Time;

    ago.textContent = formatTimeAgo(infoTime);

    likesArray[Number(div.dataset.postId - 1)] = Number(infoLikes.split(": ")[1]);
    countLikes.textContent = likesArray[Number(div.dataset.postId - 1)];
    if (infoLikes.split(": ")[0] === "true") {
        iconLikes.className = "fas fa-heart liked";
        div.dataset.liked = "true";
        likesArray[Number(div.dataset.postId - 1)]--;
    }

    countComments.textContent = Number(infoComments);

    if (extension === "png" || extension === "jpg" || extension === "jpeg" || extension === "gif" || extension === "ico") {
        img.src = url.Path;
        img.style.height = "auto";
        img.className = "feed-image";

        if (div.offsetHeight >= 950 - 2) {
            const nexts = document.createElement("div");
            nexts.className = "likes-section";

            const iconNext = document.createElement("i");
            iconNext.className = "fa fa-caret-down";
            iconNext.id = "heartIcon";

            const next = document.createElement("button");
            next.className = "like-button";
            next.id = "likeBtn";
            next.addEventListener("click", () => {
                nexts.remove();
                div.style.maxHeight = "none";
                img.style.maxHeight = "none";
                postText.style.maxHeight = "none";
                video.style.maxHeight = "none";
            });

            next.appendChild(iconNext);
            nexts.appendChild(next);
            bottom.appendChild(nexts);
        }

        const views = document.createElement("div");
        views.className = "views-section";

        const iconViews = document.createElement("i");
        iconViews.className = "far fa-eye";
        iconViews.id = "heartIcon";

        const countViews = document.createElement("h3");
        countViews.className = "count";
        countViews.textContent = Number(infoViews) + 1;

        views.appendChild(iconViews);
        views.appendChild(countViews);

        bottom.appendChild(views);
        div.appendChild(bottom);

        cops[step] = path + url.Path.replaceAll("\\", "/");

        copyButton.addEventListener("click", () => {
            const success = copyTextUsingExecCommand(cops[step]);
            if (success) {
                notification('<i class="fas fa-check-circle"></i><span id="span">Скопировано</span><i class="fas fa-image" style="opacity: 0.8;"></i>');
            }
        });
    }
    else if (extension === "txt") {
        const response = await fetch(url.Path);

        postText.textContent = await response.text();

        if (div.offsetHeight >= 950 - 2) {
            const nexts = document.createElement("div");
            nexts.className = "likes-section";

            const iconNext = document.createElement("i");
            iconNext.className = "fa fa-caret-down";
            iconNext.id = "heartIcon";

            const next = document.createElement("button");
            next.className = "like-button";
            next.id = "likeBtn";
            next.addEventListener("click", () => {
                nexts.remove();
                div.style.maxHeight = "none";
                img.style.maxHeight = "none";
                postText.style.maxHeight = "none";
                video.style.maxHeight = "none";
            });

            next.appendChild(iconNext);
            nexts.appendChild(next);
            bottom.appendChild(nexts);
        }

        const views = document.createElement("div");
        views.className = "views-section";

        const iconViews = document.createElement("i");
        iconViews.className = "far fa-eye";
        iconViews.id = "heartIcon";

        const countViews = document.createElement("h3");
        countViews.className = "count";
        countViews.textContent = Number(infoViews) + 1;

        views.appendChild(iconViews);
        views.appendChild(countViews);

        bottom.appendChild(views);
        div.appendChild(bottom);

        cops[step] = postText.textContent;

        copyButton.addEventListener("click", () => {
            const success = copyTextUsingExecCommand(cops[step]);
            if (success) {
                notification('<i class="fas fa-check-circle"></i><span id="span">Скопировано</span><i class="fas fa-image" style="opacity: 0.8;"></i>');
            }
        });
    }
    else if (extension === "mp4") {
        video.src = url.Path;
        video.style.height = "auto";
        video.className = "feed-image";

        if (div.offsetHeight >= 950 - 2) {
            const nexts = document.createElement("div");
            nexts.className = "likes-section";

            const iconNext = document.createElement("i");
            iconNext.className = "fa fa-caret-down";
            iconNext.id = "heartIcon";

            const next = document.createElement("button");
            next.className = "like-button";
            next.id = "likeBtn";
            next.addEventListener("click", () => {
                nexts.remove();
                div.style.maxHeight = "none";
                img.style.maxHeight = "none";
                postText.style.maxHeight = "none";
                video.style.maxHeight = "none";
            });

            next.appendChild(iconNext);
            nexts.appendChild(next);
            bottom.appendChild(nexts);
        }

        const views = document.createElement("div");
        views.className = "views-section";

        const iconViews = document.createElement("i");
        iconViews.className = "far fa-eye";
        iconViews.id = "heartIcon";

        const countViews = document.createElement("h3");
        countViews.className = "count";
        countViews.textContent = Number(infoViews) + 1;

        views.appendChild(iconViews);
        views.appendChild(countViews);

        bottom.appendChild(views);
        div.appendChild(bottom);

        cops[step] = path + url.Path.replaceAll("\\", "/");

        copyButton.addEventListener("click", () => {
            const success = copyTextUsingExecCommand(cops[step]);
            if (success) {
                notification('<i class="fas fa-check-circle"></i><span id="span">Скопировано</span><i class="fas fa-image" style="opacity: 0.8;"></i>');
            }
        });
    }
}
function loadImages() {
    async function sequentialLoad() {
        var step = -1;
        const loadedImages = [];
        if (array.Posts.length > 0) {
            var interval = setInterval(async () => {
                step++;

                if (step == array.Posts.length) {
                    clearInterval(interval);
                    document.getElementById("load").innerHTML = "Конец ленты";
                    return loadedImages;
                }

                var url = array.Posts[step];

                const user = url.Path.split("\\")[1];

                const login = localStorage.getItem("login");
                const password = localStorage.getItem("password");

                const address = url.Path;

                const div = document.createElement("div");
                div.className = "feed-block";
                div.dataset.postId = step;
                div.dataset.liked = "false";
                div.dataset.commented = "false";

                const top = document.createElement("div");
                top.className = "top";

                const userDiv = document.createElement("div");
                userDiv.className = "user";

                const divIcon = document.createElement("div");
                divIcon.className = "avatar-placeholder";

                const userIcon = document.createElement("i");
                userIcon.className = "far fa-user";
                divIcon.appendChild(userIcon);

                const tempElement = document.createElement("a");
                tempElement.className = "user-name";
                tempElement.textContent = user + ":";
                tempElement.href = "/profile/" + user;

                const ago = document.createElement("h6");
                ago.className = "time";

                userDiv.appendChild(divIcon);
                userDiv.appendChild(tempElement);
                userDiv.appendChild(ago);
                top.appendChild(userDiv);

                let copyed;
                const extension = url.Path.split(".").pop();

                const img = document.createElement("img");
                const postText = document.createElement("div");
                const video = document.createElement("video");

                var src;
                const copyButton = document.createElement("button");

                if (extension === "png" || extension === "jpg" || extension === "jpeg" || extension === "gif" || extension === "ico") {
                    const center = document.createElement("center");

                    const copy = document.createElement("div");
                    copy.className = "copy-section";

                    const iconCopy = document.createElement("i");
                    iconCopy.innerHTML = '<svg xmlns="http://www.w3.org/2000/svg" width="32" height="32" fill="#5f6368" viewBox="0 0 256 256" id="Flat"><path d = "M212.853,122.34277l-30.28418,30.28516a56.06576,56.06576,0,0,1-79.19629,0,12.0001,12.0001,0,0,1,16.9707-16.9707,32.03673,32.03673,0,0,0,45.25488,0l30.28418-30.28516a31.99969,31.99969,0,1,0-45.25488-45.25391L140.978,69.76562a11.99941,11.99941,0,1,1-16.96875-16.9707l9.64746-9.64746A55.99989,55.99989,0,1,1,212.853,122.34277Zm-97.834,63.89356-9.64648,9.64551a31.99969,31.99969,0,0,1-45.25488-45.25391l30.28418-30.28516a32.03673,32.03673,0,0,1,45.25488,0,12.0001,12.0001,0,0,0,16.9707-16.9707,56.06459,56.06459,0,0,0-79.19629,0L43.147,133.65723a55.99956,55.99956,0,0,0,79.19532,79.19531l9.64648-9.64551a11.99975,11.99975,0,1,0-16.96973-16.9707Z" /></svg>';
                    iconCopy.id = "heartIcon";

                    copyButton.className = "like-button";
                    copyButton.id = "likeBtn";

                    copyButton.appendChild(iconCopy);
                    copy.appendChild(copyButton);
                    top.appendChild(copy);
                    div.appendChild(top);

                    img.style.height = "0px";
                    img.preload = "auto";
                    center.appendChild(img);
                    div.appendChild(center);
                    loadedImages.push(center);
                } else if (extension === "txt") {
                    const copy = document.createElement("div");
                    copy.className = "copy-section";

                    const iconCopy = document.createElement("i");
                    iconCopy.className = "far fa-copy";
                    iconCopy.id = "heartIcon";

                    copyButton.className = "like-button";
                    copyButton.id = "likeBtn";

                    copyButton.appendChild(iconCopy);
                    copy.appendChild(copyButton);
                    top.appendChild(copy);
                    div.appendChild(top);

                    const postContent = document.createElement("div");
                    postContent.className = "post-content";

                    postText.className = "post-text";

                    postContent.appendChild(postText);
                    div.appendChild(postContent);
                } else if (extension === "mp4") {
                    const center = document.createElement("center");

                    const copy = document.createElement("div");
                    copy.className = "copy-section";

                    const iconCopy = document.createElement("i");
                    iconCopy.innerHTML = '<svg xmlns="http://www.w3.org/2000/svg" width="32" height="32" fill="#5f6368" viewBox="0 0 256 256" id="Flat"><path d = "M212.853,122.34277l-30.28418,30.28516a56.06576,56.06576,0,0,1-79.19629,0,12.0001,12.0001,0,0,1,16.9707-16.9707,32.03673,32.03673,0,0,0,45.25488,0l30.28418-30.28516a31.99969,31.99969,0,1,0-45.25488-45.25391L140.978,69.76562a11.99941,11.99941,0,1,1-16.96875-16.9707l9.64746-9.64746A55.99989,55.99989,0,1,1,212.853,122.34277Zm-97.834,63.89356-9.64648,9.64551a31.99969,31.99969,0,0,1-45.25488-45.25391l30.28418-30.28516a32.03673,32.03673,0,0,1,45.25488,0,12.0001,12.0001,0,0,0,16.9707-16.9707,56.06459,56.06459,0,0,0-79.19629,0L43.147,133.65723a55.99956,55.99956,0,0,0,79.19532,79.19531l9.64648-9.64551a11.99975,11.99975,0,1,0-16.96973-16.9707Z" /></svg>';
                    iconCopy.id = "heartIcon";

                    copyButton.className = "like-button";
                    copyButton.id = "likeBtn";

                    copyButton.appendChild(iconCopy);
                    copy.appendChild(copyButton);
                    top.appendChild(copy);
                    div.appendChild(top);

                    video.style.height = "0px";
                    video.controls = true;
                    video.loop = true;
                    center.appendChild(video);
                    div.appendChild(center);
                    loadedImages.push(center);
                }

                const bottom = document.createElement("div");
                bottom.className = "bottom-section";

                const buttons = document.createElement("div");
                buttons.className = "buttons";

                const likes = document.createElement("div");
                likes.className = "likes-section";

                const iconLikes = document.createElement("i");
                iconLikes.className = "far fa-heart";
                iconLikes.id = "heartIcon";

                const countLikes = document.createElement("h3");
                countLikes.className = "count";
                countLikes.textContent = "0";

                const like = document.createElement("button");
                like.className = "like-button";
                like.id = "likeBtn";
                like.addEventListener("click", () => {
                    if (iconLikes.className !== "fas fa-heart liked") {
                        iconLikes.className = "fas fa-heart liked";
                        countLikes.textContent = likesArray[Number(div.dataset.postId - 1)] + 1;

                        const request = {
                            Like: url.ID.toString()
                        };
                        const jsonString = JSON.stringify(request);

                        const xhrHistory = new XMLHttpRequest();
                        xhrHistory.open("POST", "/api");
                        xhrHistory.setRequestHeader("Login", login);
                        xhrHistory.setRequestHeader("Password", password);
                        xhrHistory.send(jsonString);
                    } else {
                        iconLikes.className = "far fa-heart";
                        countLikes.textContent = likesArray[Number(div.dataset.postId - 1)];

                        const request = {
                            Unlike: url.ID.toString()
                        };
                        const jsonString = JSON.stringify(request);

                        const xhrHistory = new XMLHttpRequest();
                        xhrHistory.open("POST", "/api");
                        xhrHistory.setRequestHeader("Login", login);
                        xhrHistory.setRequestHeader("Password", password);
                        xhrHistory.send(jsonString);
                    }
                });

                like.appendChild(iconLikes);
                like.appendChild(countLikes);
                likes.appendChild(like);

                const comments = document.createElement("div");
                comments.className = "likes-section";

                const iconComment = document.createElement("i");
                iconComment.className = "far fa-comment";
                iconComment.id = "heartIcon";

                const countComments = document.createElement("h3");
                countComments.className = "count";
                countComments.dataset.post_count = step;
                countComments.textContent = "0";

                const comment = document.createElement("button");
                comment.className = "like-button";
                comment.id = "likeBtn";
                comment.addEventListener("click", () => {
                    if (div.dataset.commented === "false") {
                        showComments(div.dataset.postId, url.ID.toString());
                        div.dataset.commented = "true";
                        div.style.maxHeight = "none";
                    }
                    else {
                        hideComments(div.dataset.postId);
                        div.dataset.commented = "false";
                        div.style.maxHeight = "1000px";
                    }
                });

                comment.appendChild(iconComment);
                comment.appendChild(countComments);
                comments.appendChild(comment);

                buttons.appendChild(likes);
                buttons.appendChild(comments);
                bottom.appendChild(buttons);

                feedContainer.insertAdjacentElement("beforeend", div);

                download(step, url, div, ago, countLikes, countComments, bottom, img, postText, video, iconLikes, copyButton, extension);

                if (step % 10 === 0) {
                    await waitForScrollToBottom(2500);
                }
            }, 250);
        } else {
            document.getElementById("load").innerHTML = "Постов нет...";
        }
    }
    return sequentialLoad();
}

function load() {
    const login = localStorage.getItem("login");
    const password = localStorage.getItem("password");

    if (localStorage.getItem("ID") !== null) {
        ID = localStorage.getItem("ID");
    }
    if (localStorage.getItem("login") !== null) {
        ID = localStorage.getItem("login");
    }

    document.getElementById("id").textContent = ID;

    const request = {
        Command: "Start..."
    };
    const jsonString = JSON.stringify(request);

    const xhrHistory = new XMLHttpRequest();
    xhrHistory.open("POST", "/api");
    xhrHistory.setRequestHeader("Login", login);
    xhrHistory.setRequestHeader("Password", password);
    xhrHistory.send(jsonString);
    xhrHistory.onload = function () {
        const start = xhrHistory.responseText;
        const data = JSON.parse(start);

        ID = data.ID;
        document.getElementById("id").textContent = ID + ":";
        localStorage.setItem("ID", ID);

        if (login != ID) {
            localStorage.removeItem("login");
            localStorage.removeItem("password");
        }

        array = data;
        loadImages();
    };
}

load();

document.body.addEventListener("dragover", function (e) {
    e.preventDefault();
});

document.body.addEventListener("drop", function (e) {
    e.preventDefault();
});

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

function fileName(str) {
    const extension = str.split(".").pop();

    let result = str.slice(0, -(extension.length + 1)).replace(/[^\x00-\x7F]/g, "");
    result = result.replaceAll(" ", "");
    result = result.replaceAll("/", "");
    result = result.replaceAll("\\", "");

    if (result === "") {
        const now = new Date();
        const time = String(now.getHours()).padStart(2, "0") + "-" + String(now.getMinutes()).padStart(2, "0") + "-" + String(now.getSeconds()).padStart(2, "0");
        result = "FILE-" + time;
    }

    return result + "." + extension;
}

document.getElementById("file_input").addEventListener("change", function (e) {
    new_file_post(e, document.getElementById("file_input").files[0]);
});

document.getElementById("upload_file").addEventListener("dragover", (event) => {
    event.preventDefault();
});

document.getElementById("upload_file").addEventListener("drop", function (e) {
    new_file_post(e, e.dataTransfer.files[0]);
});

function textToFile(content) {
    const now = new Date();
    const time = String(now.getHours()).padStart(2, "0") + "-" + String(now.getMinutes()).padStart(2, "0") + "-" + String(now.getSeconds()).padStart(2, "0");
    const fileNameStr = "POST " + time + ".txt";
    const blob = new Blob([content], { type: "text/plain" });
    return { blob, fileName: fileNameStr };
}

async function new_text_post() {
    const login = localStorage.getItem("login");
    const password = localStorage.getItem("password");

    if (document.getElementById("postText").value !== "" && !(/^\s*$/.test(document.getElementById("postText").value))) {
        notification('<i class="fas fa-check-circle"></i><span id="span">Публикация...</span><i class="fas fa-image" style="opacity: 0.8;"></i>');

        const userfile = textToFile(document.getElementById("postText").value);
        const attachment = new FormData();
        attachment.append("userfile", userfile.blob, userfile.fileName);

        if (await size(attachment)) {
            const xhr = new XMLHttpRequest();
            xhr.open("POST", "/upload");
            if (login && password) {
                xhr.setRequestHeader("Login", login);
                xhr.setRequestHeader("Password", password);
            }
            xhr.send(attachment);

            xhr.onload = function () {
                if (xhr.status == 429) {
                    const data = JSON.parse(xhr.responseText);
                    notification('<i class="fas fa-check-circle"></i><span id="span">' + formatTime(data.Time) + '</span><i class="fas fa-image" style="opacity: 0.8;"></i>');
                }
                else if (xhr.status == 200) {
                    location.reload();
                }
            };
            xhr.onerror = function () {
                notification('<i class="fas fa-check-circle"></i><span id="span">Не удалось загрузить пост. Попробуйте еще раз.</span><i class="fas fa-image" style="opacity: 0.8;"></i>');
            };
        } else {
            notification('<i class="fas fa-check-circle"></i><span>Текст слишком большой!</span><i class="fas fa-image" style="opacity: 0.8;"></i>');
        }
    } else {
        notification('<i class="fas fa-check-circle"></i><span>Прежде чем опубликовать пост, пожалуйста, напишите в него что-то.</span><i class="fas fa-image" style="opacity: 0.8;"></i>');
    }
}

document.getElementById("postText").addEventListener("keyup", (event) => {
    if (event.key === "Enter" && document.getElementById("postText").value != "" && !event.shiftKey && !(/^\s*$/.test(document.getElementById("postText").value))) {
        document.getElementById("postText").value = document.getElementById("postText").value.slice(0, -1);
        new_text_post();
    }
});
async function new_text_comment(id, post) {
    const login = localStorage.getItem("login");
    const password = localStorage.getItem("password");

    if (document.getElementById("text_" + id).value !== "" && !(/^\s*$/.test(document.getElementById("text_" + id).value))) {
        notification('<i class="fas fa-check-circle"></i><span id="span">Публикация...</span><i class="fas fa-image" style="opacity: 0.8;"></i>');

        const userfile = textToFile(document.getElementById("text_" + id).value);
        const attachment = new FormData();
        attachment.append("userfile", userfile.blob, userfile.fileName);

        if (await size(attachment)) {
            const xhr = new XMLHttpRequest();
            xhr.open("POST", "/upload");
            if (login && password) {
                xhr.setRequestHeader("Login", login);
                xhr.setRequestHeader("Password", password);
            }
            xhr.setRequestHeader("Comment", post);
            xhr.send(attachment);

            xhr.onload = function () {
                notification('<i class="fas fa-check-circle"></i><span id="span">Комментарий опубликован!</span><i class="fas fa-image" style="opacity: 0.8;"></i>');
                document.getElementById("text_" + id).value = "";
                document.getElementById("comments-" + id).remove();
                showComments(id, post.toString());
                document.querySelector('[data-post_count="' + id + '"]').textContent = Number(document.querySelector('[data-post_count="' + id + '"]').textContent) + 1;
            };
            xhr.onerror = function () {
                notification('<i class="fas fa-check-circle"></i><span id="span">Не удалось загрузить комментарий. Попробуйте еще раз.</span><i class="fas fa-image" style="opacity: 0.8;"></i>');
            };
        } else {
            notification('<i class="fas fa-check-circle"></i><span>Текст слишком большой!</span><i class="fas fa-image" style="opacity: 0.8;"></i>');
        }
    } else {
        notification('<i class="fas fa-check-circle"></i><span>Прежде чем добавить комментарий, пожалуйста, напишите в него что-то.</span><i class="fas fa-image" style="opacity: 0.8;"></i>');
    }
}

function initAutoPauseWithMutation() {
    function observeVideo(video) {
        const visibilityObserver = new IntersectionObserver((entries) => {
            entries.forEach(entry => {
                if (!entry.isIntersecting && !entry.target.paused) {
                    entry.target.pause();
                }
            });
        }, { threshold: 0.5 });
        visibilityObserver.observe(video);
    }

    document.querySelectorAll("video").forEach(observeVideo);
    const mutationObserver = new MutationObserver((mutations) => {
        mutations.forEach(mutation => {
            mutation.addedNodes.forEach(node => {
                if (node.nodeName === "VIDEO") {
                    observeVideo(node);
                }
                if (node.querySelectorAll) {
                    node.querySelectorAll("video").forEach(observeVideo);
                }
            });
        });
    });
    mutationObserver.observe(document.body, {
        childList: true, subtree: true
    });
}

initAutoPauseWithMutation();