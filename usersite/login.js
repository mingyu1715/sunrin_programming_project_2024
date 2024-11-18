document.getElementById("login-form").addEventListener("submit", function(event) {
    event.preventDefault();

    const username = document.getElementById("username").value;
    const password = document.getElementById("password").value;
    const errorMessage = document.getElementById("error-message");

    // 간단한 로그인 검증 (아이디는 'user', 비밀번호는 'password')
    if (username === "user" && password === "password") {
        // 로그인 성공 시 메모 목록 페이지로 이동
        window.location.href = "memo.html";
    } else {
        errorMessage.textContent = "아이디나 비밀번호가 잘못되었습니다.";
    }
});
