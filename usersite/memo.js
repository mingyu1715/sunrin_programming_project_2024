// 로그인된 사용자만 접근 가능하도록 처리
document.addEventListener("DOMContentLoaded", function() {
    // 여기에 로그인이 되어 있는지 확인하는 코드 (예: 세션 체크 등)

    // 메모 추가 기능
    document.getElementById("add-memo").addEventListener("click", function() {
        const newMemoText = document.getElementById("new-memo").value;
        if (newMemoText.trim() === "") {
            return;
        }

        const memoList = document.getElementById("memo-list");
        const newMemoItem = document.createElement("li");

        newMemoItem.innerHTML = `
            ${newMemoText}
            <button class="delete-memo">삭제</button>
        `;

        // 삭제 버튼 클릭 시 메모 삭제
        newMemoItem.querySelector(".delete-memo").addEventListener("click", function() {
            memoList.removeChild(newMemoItem);
        });

        memoList.appendChild(newMemoItem);
        document.getElementById("new-memo").value = ""; // 메모 입력란 비우기
    });

    // 로그아웃 처리
    document.getElementById("logout").addEventListener("click", function() {
        // 로그아웃 처리 후 로그인 페이지로 이동
        window.location.href = "login.html";
    });
});
