# Git 저장 및 다른 PC에서 프로젝트 진행하기

## 1. 현재 PC에서 Git 커밋 (최초 1회)

### 1-1. Git 저장소 초기화 (아직 안 했다면)

```powershell
cd "D:\05.Develop APP\03.Cursor\02.Axiom\01.Axiom-C\LSG-AX-CTL-26"
git init
```

### 1-2. 원격 저장소 연결 (GitHub/GitLab 등 사용 시)

```powershell
# 예: GitHub
git remote add origin https://github.com/YOUR_USERNAME/LSG-AX-CTL-26.git

# 또는 로컬/네트워크 경로
# git remote add origin \\SERVER\share\LSG-AX-CTL-26.git
```

### 1-3. 파일 스테이징 및 커밋

```powershell
# 모든 변경 파일 확인
git status

# 전체 추가 ( .gitignore 제외 )
git add .

# 커밋
git commit -m "Initial commit: LSG-AX-CTL-26 PC control tool, actuator firmware review"
```

### 1-4. 원격 저장소로 푸시 (원격 사용 시)

```powershell
# 기본 브랜치 이름 설정 (필요 시)
git branch -M main

# 푸시
git push -u origin main
```

---

## 2. 이후 작업 후 커밋 절차

```powershell
cd "D:\05.Develop APP\03.Cursor\02.Axiom\01.Axiom-C\LSG-AX-CTL-26"

git add .
git status                    # 추가된 파일 확인
git commit -m "작업 내용 요약"
git push                      # 원격 저장소 사용 시
```

---

## 3. 다른 PC에서 프로젝트 설정

### 3-1. 저장소 클론 (원격 사용 시)

```powershell
# 작업할 폴더로 이동 후
git clone https://github.com/YOUR_USERNAME/LSG-AX-CTL-26.git
cd LSG-AX-CTL-26
```

### 3-2. USB/네트워크로 복사한 경우

```powershell
# 프로젝트 폴더로 이동
cd "경로\LSG-AX-CTL-26"

# Git 초기화가 되어 있다면 pull만
git pull
```

### 3-3. Python 가상환경 및 패키지 설치

```powershell
# Python 3.10+ 설치 확인
python --version

# 가상환경 생성
python -m venv .venv

# 가상환경 활성화 (Windows PowerShell)
.\.venv\Scripts\Activate.ps1

# 실행 정책 오류 시:
# Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser

# 패키지 설치
pip install -r requirements.txt
```

### 3-4. 프로그램 실행

```powershell
.\.venv\Scripts\Activate.ps1
python main.py
# 또는
.\main.py
```

---

## 4. 다른 PC에서 작업 후 커밋·푸시

```powershell
git add .
git commit -m "작업 내용"
git push
```

---

## 5. 원격 저장소 없이 USB로만 동기화

1. **현재 PC**: 프로젝트 폴더 전체를 USB에 복사 (`.venv` 제외 가능 — 다른 PC에서 다시 생성)
2. **다른 PC**: USB에서 작업 폴더로 복사
3. **다른 PC**: `.venv`가 없으면 위 3-3 절차로 가상환경·패키지 설치
4. **다른 PC**에서 수정 후: 수정된 폴더를 다시 USB에 덮어쓰기
5. **현재 PC**: USB 내용을 프로젝트 폴더에 덮어쓰기

Git을 사용하면 `.venv`를 제외하고 소스만 관리할 수 있어 더 안전합니다.

---

## 6. 요약 체크리스트

### 현재 PC (저장)

- [ ] `git init` (최초 1회)
- [ ] `git remote add origin ...` (원격 사용 시)
- [ ] `git add .`
- [ ] `git commit -m "..."`
- [ ] `git push` (원격 사용 시)

### 다른 PC (설정)

- [ ] `git clone` 또는 프로젝트 폴더 복사
- [ ] `python -m venv .venv`
- [ ] `.\.venv\Scripts\Activate.ps1`
- [ ] `pip install -r requirements.txt`
- [ ] `python main.py` 실행 확인
