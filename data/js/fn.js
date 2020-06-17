function logedIn() {
    term.write("Welcome to server terminal\r\nLog^ged in from: ")

    $.get('https://www.cloudflare.com/cdn-cgi/trace', function (data) {
        let ipData = data.match(/[^\r\n]+/g)
        for (let index = 0; index < ipData.length; index++) {
            if (ipData[index].split("=")[0] === "ip")
                term.writeln(ipData[index].split("=")[1])
        }
    }).then(() => {
        term.write(`\r\n${termname} :~$ `)
    })


}

function textToBin(text) {
    return (
        Array
            .from(text)
            .reduce((acc, char) => acc.concat(char.charCodeAt().toString(2)), [])
            .map(bin => '0'.repeat(8 - bin.length) + bin)
            .join(' ')
    );
}

async function _connection(socket, timeout = 10000) {
    const isOpened = () => (socket.readyState === WebSocket.OPEN)

    if (socket.readyState !== WebSocket.CONNECTING) {
        return isOpened()
    }
    else {
        const intrasleep = 100
        const ttl = timeout / intrasleep // time to loop
        let loop = 0
        while (socket.readyState === WebSocket.CONNECTING && loop < ttl) {
            await new Promise(resolve => setTimeout(resolve, intrasleep))
            loop++
        }
        return isOpened()
    }
}

async function connect() {
    if (ws && ws.readyState === WebSocket.OPEN) {
        console.log("Disconnect")
        ws.close();
        $('#connectionstate').css("background-color", "red")
        $('#BTNconnect').text("Connect")
        return;
    }
    console.log(`Connecting to ${host}`);
    $('#connectionstate').css("background-color", "yellow")
    ws = new WebSocket(`ws://${host}/data`);
    if (await (_connection(ws))) {
        $('#connectionstate').css("background-color", "green")
        $('#BTNconnect').text("Disonnect")
    }
    else {
        $('#connectionstate').css("background-color", "red")
        $('#BTNconnect').text("Connect")

    }
}

async function logout() {
    await $.get("logout")
    .then((res) => {console.log(res);    window.location.reload();})
    .catch((ex) =>{console.log(ex) ;   window.location.reload();})

}