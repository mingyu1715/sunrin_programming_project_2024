from flask import Flask, render_template, request
import subprocess

app = Flask(__name__)

def check_login_with_c(username, password):
    try:
        # C 프로그램 실행
        result = subprocess.run(
            ['./login_system', username, password],  # C 프로그램 실행
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True  # Python 3.7+에서 'text' 옵션을 사용해 바로 문자열로 처리 가능
        )
        
        # C 프로그램의 출력값을 확인
        print(f"C Program Output: {result.stdout.strip()}")  # 디버깅용 출력
        
        output = result.stdout.strip()
        if output.isdigit():  # 출력이 숫자인지 확인
            return int(output)
        else:
            print(f"Error: C 프로그램 출력값이 숫자가 아닙니다: {output}")
            return 0  # 기본값 반환 (로그인 실패 처리)

    except Exception as e:
        print(f"Error occurred while running C program: {e}")
        return 0  # 예외 발생 시 기본값 반환

@app.route('/', methods=['GET', 'POST'])
def login():
    if request.method == 'POST':
        username = request.form['username']
        password = request.form['password']
        
        # C 프로그램을 이용한 로그인 인증
        result = check_login_with_c(username, password)
        #print(result)
        
        if result == 1:
            return "로그인 성공!"  # 성공 시 문자열 반환
        else:
            return "아이디 또는 비밀번호가 틀립니다."  # 로그인 실패 시 메시지 반환

    return render_template('login.html')

if __name__ == '__main__':
    app.run(debug=True)
