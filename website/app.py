from flask import Flask, render_template, request, redirect, url_for, flash, session , jsonify
import subprocess
import os

app = Flask(__name__)
app.secret_key = 'your_secret_key'  # flash()를 사용하려면 비밀키가 필요합니다.

# 로그인 체크 함수
def check_login_with_c(username, password):
    try:
        result = subprocess.run(
            ['./user_management', username, password, 'login'],  # 'login' 액션
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )
        
        output = result.stdout.strip()
        if output.lstrip('-').isdigit():
            return int(output)
        else:
            return 0

    except Exception as e:
        return 0

# 사용자 등록 함수
def register_user_with_c(username, password):
    try:
        result = subprocess.run(
            ['./user_management', username, password, 'register'],  # 'register' 액션
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )

        output = result.stdout.strip()
        if output.lstrip('-').isdigit():
            return int(output)
        else:
            return 0  # 실패

    except Exception as e:
        return False
    
import subprocess
import os

def decrypt_memo(memo_id, username):
    try:
        # 임시 txt 파일 생성
        temp_filename = f"output_file.txt"
        encrypt_file = f"data/userdata/{username}/{memo_id}.txt"

        # C 프로그램 호출 (memo_id와 username 전달, 출력 파일 경로로 temp_filename 전달)
        result = subprocess.run(
            ['./decrypt', username, str(memo_id), encrypt_file, temp_filename],  # C 프로그램과 인자 (출력 파일 경로 추가)
            stdout=subprocess.PIPE, 
            stderr=subprocess.PIPE,
            text=True  # stdout과 stderr를 텍스트로 반환하도록 설정
        )

        # C 프로그램의 출력과 오류를 로그에 기록
        print("C 프로그램 출력:", result.stdout)  # C 프로그램의 stdout 출력
        print("C 프로그램 오류:", result.stderr)  # C 프로그램의 stderr 출력

        # C 프로그램 실행이 실패했을 경우, 예외 처리
        if result.returncode != 0:
            print(f"Error: C program returned non-zero exit status {result.returncode}")
            print(f"Error message: {result.stderr.strip()}")
            return None

        # 임시 파일에서 복호화된 내용 읽기
        with open(temp_filename, 'r') as f:
            memo_content = f.read().strip()

        # txt 파일 삭제
        os.remove(temp_filename)

        return memo_content

    except subprocess.CalledProcessError as e:
        print(f"Error during decryption: {e}")
        return None  # 복호화 중 오류 발생 시 None 반환

    except Exception as e:
        print(f"Unexpected error: {e}")
        return None  # 예기치 못한 오류 발생 시 None 반환


# 메모 내용 보기 라우트
@app.route('/view_note', methods=['GET'])
def view_note():
    memo_id = request.args.get('memo_id')  # URL에서 memo_id를 가져옵니다.
    username = session['username']
    
    if not memo_id or not username:
        return jsonify({"error": "메모 ID와 사용자명을 제공해야 합니다."}), 400
    
    # memo_id가 숫자형인지 체크
    if not memo_id.isdigit():
        return jsonify({"error": "메모 ID는 숫자여야 합니다."}), 400

    memo_content = decrypt_memo(int(memo_id), username)
    
    if memo_content is None:
        return jsonify({"error": "메모 복호화 실패"}), 500
    
    return render_template('view_memo.html', memo_content=memo_content)
    
@app.route('/', methods=['GET', 'POST'])
def login():
    if request.method == 'POST':
        username = request.form['username']
        password = request.form['password']
        
        result = check_login_with_c(username, password)
        
        if result == 1:
            # 세션에 사용자 이름 저장
            session['username'] = username
            
            # 로그인 후 메모 목록으로 리디렉트
            return redirect(url_for('memo_list'))
        
        elif result == -2:
            flash("유저 정보를 찾을 수 없습니다.")
            return redirect(url_for('login'))
        elif result == -1:
            flash("로그인 실패")
            return redirect(url_for('login'))

    return render_template('login.html')

@app.route('/memo_list', methods=['GET'])
def memo_list():
    username = session['username']
    # C 프로그램 실행
    result = subprocess.run(["./memo_list", username], capture_output=True, text=True)
    
    if result.returncode != 0:
        return f"Error: {result.stderr}"
    
    # C 프로그램의 출력을 처리
    memos = result.stdout.strip().split("\n")  # 줄 바꿈을 기준으로 분리

    # 출력 확인 (디버깅 용도)
    print("Memos:", memos)
    
    return render_template('memo_list.html', memos=memos, username=username)

# 새 메모 작성 페이지
@app.route('/create_note', methods=['GET'])
def create_note():
    return render_template('create_note.html')

# 메모 저장
@app.route('/save_note', methods=['POST'])
def save_note():
    if 'username' not in session:
        flash("로그인이 필요합니다.")
        return redirect(url_for('login'))

    username = session['username']  # 세션에서 로그인한 사용자 이름 가져오기
    print(f"Logged in user: {username}")  # 로그로 사용자 확인

    title = request.form['title']
    data = request.form['data']

    # 사용자별 디렉토리 경로 설정
    user_directory = f"data/userdata/{username}"
    
    # 디렉토리가 없으면 생성
    if not os.path.exists(user_directory):
        os.makedirs(user_directory)

    # 저장할 파일 경로
    input_file = "input.txt"
    output_file = os.path.join(user_directory, f"{title}.txt")  # 암호화된 파일은 사용자 디렉토리에 저장

    # 데이터를 임시 파일에 저장
    with open(input_file, 'w', encoding='utf-8') as f:
        f.write(data)

    # C 프로그램을 실행하여 암호화
    result = subprocess.run(
        ['./encrypt', username, input_file, title],  # C 프로그램 실행
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE
    )

    if result.returncode != 0:
        error_message = result.stderr.decode('utf-8')  # 오류 메시지 디코딩
        print(f"Error during encryption: {error_message}")  # 서버 로그에 출력
        return f"Error: {error_message}", 500  # 에러 메시지를 클라이언트로 반환

    # 임시 파일 삭제
    os.remove(input_file)

    # 저장이 완료되면 메모 목록으로 리다이렉트
    return redirect(url_for('memo_list'))

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

@app.route('/edit_note', methods=['GET', 'POST'])
def edit_note():
    if 'username' not in session:
        flash("로그인이 필요합니다.")
        return redirect(url_for('login'))

    username = session['username']
    memo_id = request.args.get('memo_id')  # 메모 ID 가져오기
    
    if not memo_id or not memo_id.isdigit():
        flash("잘못된 메모 ID입니다.")
        return redirect(url_for('memo_list'))

    encrypted_file_path = f"data/userdata/{username}/{memo_id}.txt"
    decrypted_file_path = f"temp_file.txt"

    # GET 요청: 기존 메모 복호화 후 보여주기
    if request.method == 'GET':
        memo_content = decrypt_memo(memo_id,username)

        return render_template('edit_note.html', memo_content=memo_content, memo_id=memo_id)

    # POST 요청: 수정된 메모 저장
    elif request.method == 'POST':
        updated_content = request.form['content']  # 수정된 메모 내용

        # 수정된 내용을 임시 파일로 저장
        with open(decrypted_file_path, 'w', encoding='utf-8') as f:
            f.write(updated_content)

        # C 프로그램으로 암호화
        result = subprocess.run(
            ['./encrypt', username, decrypted_file_path, memo_id], 
            stdout=subprocess.PIPE, 
            stderr=subprocess.PIPE
        )

        if result.returncode != 0:
            error_message = result.stderr.decode('utf-8')
            flash(f"암호화 오류: {error_message}")
            return redirect(url_for('memo_list'))

        # 임시 파일 삭제
        os.remove(decrypted_file_path)

        flash("메모가 성공적으로 업데이트되었습니다!")
        return redirect(url_for('memo_list'))

if __name__ == '__main__':
    app.run(debug=True)
