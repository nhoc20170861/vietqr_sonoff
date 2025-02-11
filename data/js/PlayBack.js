// function show toast message
function showToasstMessage(message, originClass, replaceClass) {
  $("#toast-notify").on("show.bs.toast", function () {
    const toast_header = $("#toast-notify .toast-header");
    if (toast_header.hasClass(originClass))
      toast_header.removeClass(originClass).addClass(replaceClass);
    $("#toast-notify .toast-body").text(message);
  });
  $("#toast-notify").toast("show");
}

// Toast function
function toast({ title = "", message = "", type = "info", duration = 3000 }) {
  const main = document.getElementById("toast__custom");
  if (main) {
    const toast = document.createElement("div");

    // Auto remove toast
    const autoRemoveId = setTimeout(function () {
      main.removeChild(toast);
    }, duration + 1000);

    // Remove toast when clicked
    toast.onclick = function (e) {
      if (e.target.closest(".toast__close")) {
        main.removeChild(toast);
        clearTimeout(autoRemoveId);
      }
    };

    const icons = {
      success: "fas fa-check-circle",
      info: "fas fa-info-circle",
      warning: "fas fa-exclamation-circle",
      error: "fas fa-exclamation-circle",
    };
    const icon = icons[type];
    const delay = (duration / 1000).toFixed(2);
    const timePusher = updateDateTime();

    toast.classList.add("toast__custom", `toast--${type}`);

    toast.style.animation = `slideInLeft ease 0.2s, fadeOut linear 1s ${delay}s forwards`;
    toast.innerHTML += `
   
                    <div class="toast__icon">
                        <i class="${icon}"></i>
                    </div>
                    <div class="toast__body">
                      <div class="toast__begin">
                        <h3 class="toast__title">${title}</h3>
                        <small class="text-muted"> ${timePusher} </small>
                      </div>
                        <p class="toast__msg">${message}</p>
                    </div>
                    <div class="toast__close">
                        <i class="fas fa-times"></i>
                    </div>
                `;
    main.appendChild(toast);
  }
}
var array;
function showSuccessToast(message) {
  toast({
    title: "Success!",
    message: message,
    type: "success",
    duration: 5000,
  });
}

function showErrorToast(message) {
  toast({
    title: "Failure!",
    message: message,
    type: "error",
    duration: 5000,
  });
}
function showWarningToast(message) {
  toast({
    title: "Warning!",
    message: message,
    type: "warning",
    duration: 5000,
  });
}
function showInfoToast(message) {
  toast({
    title: "Info!",
    message: message,
    type: "info",
    duration: 5000,
  });
}

//   source.addEventListener(
//     "addChannel",
//     function (e) {
//       // conver json to object
//       // showToasstMessage("Add channel success!", "bg-danger", "bg-success");
//       showSuccessToast("Add channel success!");
//       const myObject = JSON.parse(e.data);
//       length_ChannelList += 1;
//       console.log(myObject);
//       // const myArray = JSON.parse(localStorage.getItem("channelList"));

//       let html =
//         '<tr  onclick="add(this)" ' +
//         'id="tbody_tr_' +
//         length_ChannelList +
//         '">' +
//         '<td id="td-id">' +
//         length_ChannelList +
//         "</td>" +
//         '<td id="td-ssid">' +
//         myObject["ssid"] +
//         ` <td id="td-action">
//         <button
//           onclick="showEditModal(this)"
//           type="button"
//           class="btn btn-info btn-sm"
//           data-toggle="tooltip"
//           data-original-title="Edit"
//           data-placement="top"
//         >
//           <i
//             class="fa-regular fa-pen-to-square"
//           ></i>
//         </button>
//         <button
//           type="button"
//           class="btn btn-warning btn-sm"
//           onclick="showDeleteModal(this)"
//           data-toggle="tooltip"
//           data-original-title="Delete"
//           data-placement="top"
//         >
//           <i
//             class="fa-regular fa-trash-can"
//           ></i>
//         </button>
//       </td>` +
//         "</tr>";

//       $("#table-tbody").append(html);
//       // $('[data-toggle="tooltip"]').tooltip();
//     },
//     false
//   );
// }

var current_channel_id_selected = 0;
var current_channel_id_playing = 0;
// function to showEditModal when click button Edit
function showEditModal(element) {
  //$('[data-toggle="tooltip"]').tooltip("hide");

  const parentElem = element.parentElement;

  current_channel_id_selected =
    parentElem.parentElement.firstElementChild.textContent;

  $("#modalForEdit").on("show.bs.modal", function () {
    const modal_title = $("#modalForEdit .modal-title");
    modal_title.text("Chỉnh sửa WiFi có ID: " + current_channel_id_selected);
    const ssidEditInput = document.getElementById("WiFiEditName");
    const ssidEditName = parentElem.parentElement.children[1].textContent;
    console.log(ssidEditName);
    ssidEditInput.value = ssidEditName;
  });

  $("#modalForEdit").modal();
}
// function to showEditModal when click button Delete
function showDeleteModal(element) {
  //$('[data-toggle="tooltip"]').tooltip("hide");

  const parentElem = element.parentElement;

  current_channel_id_selected =
    parentElem.parentElement.firstElementChild.textContent;

  $("#modalForDelete").on("show.bs.modal", function () {
    const modal_title = $("#modalForDelete .modal-title");
    modal_title.text("Xoá cấu hình WiFi: " + current_channel_id_selected);
  });

  $("#modalForDelete").modal();
}
//Function to add date and time of last update
function updateDateTime() {
  var currentdate = new Date();
  let currentHour =
    currentdate.getHours() < 10
      ? `0${currentdate.getHours()}`
      : currentdate.getHours();
  let currentMinute =
    currentdate.getMinutes() < 10
      ? `0${currentdate.getMinutes()}`
      : currentdate.getMinutes();
  let currentSecond =
    currentdate.getSeconds() < 10
      ? `0${currentdate.getSeconds()}`
      : currentdate.getSeconds();
  var datetime = currentHour + ":" + currentMinute + ":" + currentSecond;
  // document.getElementById("update-time").innerHTML = datetime;
  // console.log(datetime);
  return datetime;
}

$(document).ready(function () {
  $("form").each(function () {
    let form = $(this);
    form.submit(async function (e) {
      e.preventDefault();
      let actionUrl = form.attr("action");
      let dataToSend = form.serialize();
      let indexChannelEdit = current_channel_id_selected - 1;
      if (actionUrl == "/editWiFiConFig") {
        await $("#modalForEdit").modal("hide");
        dataToSend =
          "IndexChannelEdit=" + +indexChannelEdit + "&" + form.serialize();
      }
      console.log(dataToSend);
      $.ajax({
        type: "POST",
        url: actionUrl,
        data: dataToSend, // serializes the form's elements.
        success: async function (data) {
          console.log(data);
          if (actionUrl == "/editWiFiConFig") {
            const object = JSON.parse(data);
            console.log("🚀 ~ object:", object);

            const queryTdSSID =
              "#tbody_tr_" + current_channel_id_selected + " #td-ssid";

            await $(queryTdSSID).text(object.new_ssid);
            showSuccessToast("Sửa WiFi thành công");
          } else {
            showSuccessToast("Thêm WiFi thành công");
          }
          const timeoutID3 = setTimeout(() => {
            location.reload();
            clearTimeout(timeoutID3);
          }, 1000);
        },
      });
    });
  });
});

// jquery code for render channel list
var length_ChannelList = 0;
var array = [];
function renderWiFiList(wifiList, element) {
  let html = "";
  let arrKey = Object.keys(wifiList);
  length_ChannelList = arrKey.length;
  for (let i = 0; i < length_ChannelList; i++) {
    if (wifiList[arrKey[i]] == true) {
      current_channel_id_playing = i + 1;
      console.log(
        "🚀 ~ renderWiFiList ~ current_channel_id_playing:",
        current_channel_id_playing
      );
      html +=
        '<tr id="tbody_tr_' +
        `${i + 1}` +
        '"><td id="td-id">' +
        `${i + 1}` +
        "</td>" +
        '<td id="td-ssid">' +
        arrKey[i] +
        "</td>" +
        ` <td id="td-rssi">
  <button
    type="button"
    class="btn btn-info btn-sm"
    id="btn_showEditModal"
    onclick="showEditModal(this)"
    disabled
  >
    <i class="fa-regular fa-pen-to-square"></i>
  </button>
  <button
    type="button"
    class="btn btn-warning btn-sm"
    id="btn_showDeleteModal"
    onclick="showDeleteModal(this)"
    disabled
  >
    <i class="fa-regular fa-trash-can"></i>
  </button>
</td>` +
        "</tr>";
    } else {
      html +=
        // '<tr onclick="add(this)">' +
        '<tr id="tbody_tr_' +
        `${i + 1}` +
        '"><td id="td-id">' +
        `${i + 1}` +
        "</td>" +
        '<td id="td-ssid">' +
        arrKey[i] +
        "</td>" +
        ` <td id="td-rssi">
  <button
    type="button"
    class="btn btn-info btn-sm"
    id="btn_showEditModal"
    onclick="showEditModal(this)"
  >
    <i class="fa-regular fa-pen-to-square"></i>
  </button>
  <button
    type="button"
    class="btn btn-warning btn-sm"
    id="btn_showDeleteModal"
    onclick="showDeleteModal(this)"

  >
    <i class="fa-regular fa-trash-can"></i>
  </button>
</td>` +
        "</tr>";
    }
  }
  element.innerHTML += html;
  // $('[data-toggle="tooltip"]').tooltip();
}
// var timeOutIDGetChannelList;
$(document).ready(function () {
  // const timeoutID = setTimeout(async () => {

  const tb_body = document.getElementById("table-tbody");
  const timeOutIDGetChannelList = setTimeout(() => {
    if (tb_body) {
      tb_body.removeChild(tb_body.firstElementChild);
    }

    $.get("/getWiFiList", {}, async function (response) {
      console.log(response || "WiFi list is empty!");
      if (response != "") {
        let res = await JSON.parse(response);

        console.log(res);
        renderWiFiList(res, tb_body);
      }
    });

    clearTimeout(timeOutIDGetChannelList);
  }, 1000);
});

// check when button comfirm delete channel press
$(document).ready(function () {
  $("#btn_confirm_deleteChannel").click(async () => {
    console.log(
      "🚀 ~ $ ~ current_channel_id_playing:",
      current_channel_id_playing
    );
    console.log(
      "🚀 ~ $ ~ current_channel_id_selected:",
      current_channel_id_selected
    );
    if (current_channel_id_playing == current_channel_id_selected) {
      showToasstMessage(
        "Không được xoá WiFi đang kết nối",
        "bg-success",
        "bg-danger"
      );
      // showErrorToast("Can't delete a running channel");
    } else {
      const indexChannelDelete = +current_channel_id_selected - 1;

      console.log("btn_confirm_deleteChannel");
      await $.get(
        "/deleteChannel",
        {
          IndexChannelDelete: indexChannelDelete,
        },
        function (response) {
          console.log(response);
          const timeoutID2 = setTimeout(() => {
            location.reload();
            clearTimeout(timeoutID2);
          }, 1000);
        }
      );
    }
  });
});

$(document).ready(async function () {
  $("#btn_clear").click(() => {
    console.log("clear form");
    $("#ssid").val("");
    $("#pass").val("");
  });
});
