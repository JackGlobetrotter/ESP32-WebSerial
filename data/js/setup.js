var host = window.location.hostname
var ws;
const termname = "server"
var term = new Terminal();
var curr_line = "";
var curr_bck = "";
var entries = [];
var incomming = "";

async function create() {
    term.prompt = () => {
        term.write(`\r\n`)
    };

    $('#connectionstate').css("background-color", "red")

    $('#IN_IP').val(host)
    $('#BTNreboot').click(() => { $.get(`reboot`) })
    $('#IN_IP').change((e) => { host = e.target.value })
    $('#BTNconnect').click(() => {connect() })
    $('#BTNlogout').click(() => { logout() })
    await connect();

    // Listen for messages
    ws.addEventListener('message', async function (event) {
        try {
            let data = new TextDecoder("utf-8").decode(new Uint8Array(await event.data.arrayBuffer()))
            if (curr_line !== "") {
                curr_line = "";
                term.select(0, term.rows, curr_line.length)
                term.clearSelection();
            }
            term.write(data);

        }
        catch (ex) { console.log(ex); console.log(event) }
    });
    ws.onerror = function (ev) {
        console.log(ev)
        $('#connectionstate').css("background-color", "red")
    }
    window.onbeforeunload = function () {
        ws.onclose = function () { }; // disable onclose handler first
        ws.close();
        logout();
    };
    ws.onopen = () => console.log("connected")
    //ws.addEventListener("open", ()=>{            ws.send('\x04')})

    term.open(document.getElementById('terminal-container'));
    term.writeln("Welcome to server terminal\r\n")

    term.on("key", function (key, ev) {
        //Enter
        if (ev.ctrlKey === true) {
            switch (ev.keyCode) {
                case 65: //CTRL-A
                    ws.send('\x01'); break;
                case 66: //CTRL-B
                    ws.send('\x02'); break;
                case 67://CTRL-C
                    ws.send('\x03'); break;
                case 68://CTRL-D
                    ws.send('\x04'); break;
                case 75://CTRL-K
                    ws.send('\x11'); break;
                case 87: //CTRL-W
                    ws.send('\x23'); break;
                case 88: //CTRL-X
                    ws.send('\x24'); break;
                default:
                    console.log(ev.keyCode)
            }
        }
        else if (ev.keyCode === 13) {
            ws.send("\r\n")
        }
        else if (ev.keyCode === 8)
            ws.send("\b")
        else if (ev.keyCode === 9)
            ws.send("\t")
        else if (ev.keyCode >= 65 && ev.keyCode <= 90) //text
            ws.send(event.shiftKey ? String.fromCharCode(event.keyCode).toUpperCase() : String.fromCharCode(event.keyCode).toLocaleLowerCase())
        else
            ws.send(ev.key)
        return;
    });

    term.on("paste", function (data) {
        curr_line += data;
        term.write(data);
    });

}

