from flask import Flask, render_template, request, redirect, url_for, flash
import subprocess

app = Flask(__name__)
app.secret_key = 'your_secret_key'  # flash()를 사용하려면 비밀키가 필요합니다.

def check_login_with_c(username, password):
    try:
        result = subprocess.run(
            ['./login_system', username, password, 'login'],  # 'login' 액션
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )
        print(result)
        
        output = result.stdout.strip()
        print(output)
        if output.lstrip('-').isdigit():
            return int(output)
        else:
            return 0

    except Exception as e:
        return 0

def register_user_with_c(username, password):
    try:
        result = subprocess.run(
            ['./login_system', username, password, 'register'],  # 'register' 액션
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )

        print(result)
        
        output = result.stdout.strip()
        print(output)
        if output.lstrip('-').isdigit():
            return int(output)
        else:
            return 0  # 실패

    except Exception as e:
        return False

@app.route('/', methods=['GET', 'POST'])
def login():
    if request.method == 'POST':
        username = request.form['username']
        password = request.form['password']
        
        result = check_login_with_c(username, password)
        
        if result == 1:
            memos = ["메모 1", "메모 2", "메모 3"]
            return render_template('memo_list.html', username=username, memos=memos)
        elif result == -2:
            flash("유저 정보를 찾을 수 없습니다.")
            return redirect(url_for('login'))
        elif result == -1:
            flash("로그인 실패")
            return redirect(url_for('login'))

    return render_template('login.html')

@app.route('/create_note', methods=['GET'])
def create_note():
    # 새 메모 작성 페이지로 이동
    return render_template('create_note.html')

@app.route('/register', methods=['GET', 'POST'])
def register():
    if request.method == 'POST':
        username = request.form['username']
        password = request.form['password']
        re_password = request.form['re_password']  # 비밀번호 확인

        # 비밀번호 길이 체크
        if len(password) < 8:
            flash("비밀번호는 최소 8자 이상이어야 합니다.")
            return redirect(url_for('register'))  # 비밀번호 길이 부족

        # 비밀번호 확인
        if password != re_password:
            flash("비밀번호가 일치하지 않습니다.")
            return redirect(url_for('register'))  # 비밀번호 불일치 메시지

        # C 프로그램을 통해 사용자 추가
        success = register_user_with_c(username, password)

        print(success)

        if success == 1:
            flash("가입 성공!")
            return redirect(url_for('login'))  # 가입 성공 후 로그인 페이지로 리디렉션
        elif success == -3:
            flash("계정 이미 있음")
            return redirect(url_for('register'))
        else:
            flash("가입 실패, 다시 시도해 주세요.")
            return redirect(url_for('register'))  # 가입 실패 메시지

    return render_template('register.html')

if __name__ == '__main__':
    app.run(debug=True)
