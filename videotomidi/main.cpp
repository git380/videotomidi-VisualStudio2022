#include <opencv2/opencv.hpp>

using namespace cv;
using namespace std;

class Movie {
private:
    VideoCapture cap; // �����ǂݍ��ނ��߂̃o�b�t�@
    Mat img; // �摜�p�̃o�b�t�@�A���悩��P�t���[�������o��
    int max_frame; // ����̑��t���[����
    int cur_frame_num = 0; // ���ݓǂݍ���ł���t���[��

public:
    Movie() : cap("C:\\opencv\\video.mp4"), max_frame(cap.get(CAP_PROP_FRAME_COUNT)) {
        // �I�t�Z�b�g���̃t���[�����΂�
        for (int i = 0; i < Get_FPS() * 3; i++) if (cur_frame_num < max_frame) cap >> img;
    }
    Mat Get_Next_Frame() { if (cur_frame_num < max_frame) cap >> img; return img; } // ���̃t���[�����擾����֐�
    int Get_FPS() { return cap.get(CAP_PROP_FPS); } // ����̃t���[�����[�g���擾����֐�
    int Get_Max_Frame() { return max_frame; } // ����̑��t���[�������擾����֐�
    int Cur_Frame_Num() { return cur_frame_num; } // ���݂̃t���[���ԍ����擾����֐�
};

class Analysis {
private:
    const static int threshold = 50;// �F���ǂꂭ�炢�ς������ω������ƔF�����邩��臒l
    Movie movie; // ����̏������s���C���X�^���X
    cv::Mat frame; // �t���[����ێ�����Mat�I�u�W�F�N�g

    // ���W�̕ێ�
    // ����
    int key_white_y; // �c��
    int key_white_x[52]; // ����
    // ����
    int key_black_y; // �c��
    int key_black_x[36]; // ����
    // 88���ՑS��
    int key_x[88];

    int active_key_sum = 0; // ��ʕύX�̎��̌��m�΍�(�ω��ʂ���������ƁA�����o�����Ȃ��t���O�݂����Ȏg����)
    int first_key = 0; // ���߂ẴL�[�C�x���g�̃t���O

    // �e���Ղ�������ĂȂ�������Ԃ�RGB�l��ێ�����ϐ�
    // ����
    int def_w_clrB, def_w_clrG, def_w_clrR;
    // ����
    int def_b_clrB, def_b_clrG, def_b_clrR;

    // �e���Ղ̍��W�̃C�x���g��ԁi�����ꂽ���ǂ����j��ێ�����z��
    bool key_w_event[52] = { false }; // ����
    bool key_b_event[36] = { false }; // ����
    bool key_event[88] = { false }; // 88���ՑS�̂̃C�x���g��Ԃ�ێ�����z��

    string str = ""; // 1�t���[������ێ����镶����
    string str_ = ""; // ����1�t���[�����ɖ�肪�Ȃ���΁A��������


public:
    Analysis(); // �R���X�g���N�^
    void Set_Coodinates(); // ���Ղ̍��W��ݒ肷��֐�
    void Set_Color(); // ���Ղ�������Ă��Ȃ�������Ԃ�RGB���L�^����֐�
    void Analyze(); // �f���̉�͂��s���Atxt�t�@�C���𐶐�����֐�
    void Check_Coodinates(); // ��ʂ̍��W���`�F�b�N����֐�

    // �e���Ղ̐F���ς�������`�F�b�N����֐�(�F���ς���Ă�����true��Ԃ�)
    bool Change_Color_w(int b, int g, int r);// ����
    bool Change_Color_b(int b, int g, int r);// ����

    void Check_Key(); // �e���Ղ̏�Ԃ��`�F�b�N����֐�

    // �w�肳�ꂽ���W�̏����擾����֐�
    int Get_Color_b(int x, int y); // �F
    int Get_Color_g(int x, int y); // �ΐF
    int Get_Color_r(int x, int y); // �ԐF

    bool True_White(int n); // ���Ղ��������ǂ����𔻒肷��֐�
    void Register_Event(int key, int event); // �e���Ղ̃C�x���g�i�����ꂽ�������ꂽ���j���L�^����֐�
    void Output_txt(); // ��͌��ʂ��e�L�X�g�t�@�C���ɏ����o���֐�
};

// Analysis�N���X�̃R���X�g���N�^
Analysis::Analysis() {
    Set_Coodinates(); // ���Ղ̍��W��ݒ肷��֐��̌Ăяo��
    Set_Color(); // ���Ղ�������Ă��Ȃ���Ԃ�RGB�l���L�^����֐��̌Ăяo��
}

// ���Ղ̍��W��ݒ肷��֐�
void Analysis::Set_Coodinates() {
    cout << "Set_Coodinates() ���W�Z�b�g�֐�" << endl;

    // 720p 88���� �̍��W�ݒ�
    cout << "Mode = 720p 88 key" << endl;

    // ����
    // Y���W(���Ղ́A���ɕ���ł���̂ŏc���͕ύX�Ȃ�)
    key_white_y = 665;
    // X���W���v�Z
    for (int i = 0; i < 52; i++) key_white_x[i] = (24.5 / 2.0) + i * 24.6;

    // ����
    // Y���W(���Ղ́A���ɕ���ł���̂ŏc���͕ύX�Ȃ�)
    key_black_y = 620;
    // X���W���v�Z
    key_black_x[0] = 28.5; // ��ԍŏ��̍������W�ݒ�
    int k = 0; // 1�I�N�^�[�u���̍����ϐ�
    int def = 27; // �������W�ϐ�
    double W_1 = 43.2; // �Ԃɍ������Ȃ�
    double W_2 = 29.3; // �Ԃɍ���������
    for (int i = 1; i < 36; i++) {
        switch (k) {
        case 0: def += W_1; break; // #�h
        case 1: def += W_2; break; // #��
        case 2: def += W_1; break; // #�t�@
        case 3: def += W_2; break; // #�\
        case 4: // #��
            def += W_2;
            k = -1;// 1�I�N�^�[�u������ �����ϐ����Z�b�g
            break;
        }
        k++; // ���̍���
        key_black_x[i] = def; // �������W��o�^
    }

    // 88���ՑS�̂̍��W��ێ�����z���ݒ�
    int white = 0, black = 0;
    for (int num = 0; num < 88; num++) {
        if (True_White(num)) { // �������������𔻒肵�A�Ή�����X���W��ݒ�
            // ����
            key_x[num] = key_white_x[white];
            white++;
            cout << num << "w";
        }
        else {
            // ����
            key_x[num] = key_black_x[black];
            black++;
            cout << num << "b";
        }
    }

    cout << "" << endl;

    // X���W�������Ƀ\�[�g
    sort(key_x, key_x + 88);

    // ���W�̊m�F���s���֐����Ăяo��
    Check_Coodinates();
}

// ���Ղ�������Ă��Ȃ�������Ԃ�RGB�l���L�^����֐�
void Analysis::Set_Color() {
    cout << "Set_Color() �f�t�H���g�J���[�擾�֐�" << endl;

    // �e���Ղ̏�����Ԃ�RGB�l���L�^
    // ����
    def_w_clrB = frame.at<Vec3b>(key_white_y, key_white_x[0])[0];
    def_w_clrG = frame.at<Vec3b>(key_white_y, key_white_x[0])[1];
    def_w_clrR = frame.at<Vec3b>(key_white_y, key_white_x[0])[2];
    // ����
    def_b_clrB = frame.at<Vec3b>(key_black_y, key_black_x[0])[0];
    def_b_clrG = frame.at<Vec3b>(key_black_y, key_black_x[0])[1];
    def_b_clrR = frame.at<Vec3b>(key_black_y, key_black_x[0])[2];

    cout << "B:" << def_w_clrB << ",G:" << def_w_clrG << ",R:" << def_w_clrR << endl;
    cout << "B:" << def_b_clrB << ",G:" << def_b_clrG << ",R:" << def_b_clrR << endl;
}

// �f���̉�͂��s���Atxt�t�@�C���𐶐�����֐�
void Analysis::Analyze() {
    const static double fps = movie.Get_FPS();
    // 1�t���[������������
    for (int frame_count = 1;; frame = movie.Get_Next_Frame()) {
        if (frame.empty()) break;

        Check_Key(); // �L�[�̃C�x���g���A�b�v�f�[�g

        double time_now = (double)frame_count / fps; // 1�t���[����fps�Ŋ��邱�ƂŁA���Ԃ��o��

        // ���������������ȉ��ŃC�x���g��{�o�^
        if (str.size() > 0 && str.size() < 30) {
            cout << std::to_string(time_now) << endl;
            str_ += std::to_string(time_now) + "ms" + str + "\n";
        }
        str = "";
        frame_count++; // �������I������̂��ɁA���̃t���[����ǉ�����
        first_key = 0;
        active_key_sum = 0;
        cv::imshow("movie", frame);
    }
    Output_txt(); // �Ō�Ƀt�@�C���ɏ����o��
}

// ���W�̃`�F�b�N���s���֐�
void Analysis::Check_Coodinates() {
    cout << "Check_Coodinates() ���W�`�F�b�N�֐�" << endl;

    cv::namedWindow("movie", cv::WINDOW_AUTOSIZE);

    frame = movie.Get_Next_Frame();

    // �����̍��W��ԐF�̉~�ŕ\��
    for (const auto& e : key_white_x) {
        cout << e << endl;
        cv::circle(frame, cv::Point(e, key_white_y), 3, cv::Scalar(0, 0, 200), 3, 4);
    }
    // �����̍��W��ΐF�̉~�ŕ\��
    for (const auto& e : key_black_x) {
        cout << e << endl;
        cv::circle(frame, cv::Point(e, key_black_y), 3, cv::Scalar(0, 200, 0), 3, 4);
    }
    int i = 0;
    // 88���ՑS�̂̍��W�����F�̉~�ŕ\��
    for (const auto& e : key_x) {
        cout << i << "^" << e;
        cv::circle(frame, cv::Point(e, key_black_y + 30), 3, cv::Scalar(255, 255, 0), 3, 4);
        i += 1;
    }

    cv::imshow("movie", frame);

    cv::waitKey(0);
}

// �F���ς�������`�F�b�N����֐�
// ����
bool Analysis::Change_Color_w(int b, int g, int r) {
    int diff = abs(b - def_w_clrB) + abs(g - def_w_clrG) + abs(r - def_w_clrR);
    return diff > threshold;
}
// ����
bool Analysis::Change_Color_b(int b, int g, int r) {
    int diff = abs(b - def_b_clrB) + abs(g - def_b_clrG) + abs(r - def_b_clrR);
    return diff > threshold;
}

// �L�[�̃C�x���g���`�F�b�N����֐�
void Analysis::Check_Key() {
    int x, y, key;

    for (key = 0; key < 88; key++) {
        x = key_x[key];
        if (True_White(key)) {
            // ����
            y = key_white_y;
            if (!key_event[key]) {
                // �����̐F���ς�����Ƃ�
                if (Change_Color_w(Get_Color_b(x, y), Get_Color_g(x, y), Get_Color_r(x, y))) {
                    cout << "[" << key << "W:on]" << endl;
                    cv::circle(frame, cv::Point(x, y), 3, cv::Scalar(0, 0, 200), 3, 4);
                    Register_Event(key, 1);
                    key_event[key] = true;
                }
            }
            else {
                // �����̐F�����ɖ߂����Ƃ�
                if (!Change_Color_w(Get_Color_b(x, y), Get_Color_g(x, y), Get_Color_r(x, y))) {
                    cout << "[" << key << "W:off]" << endl;
                    cv::circle(frame, cv::Point(x, y), 3, cv::Scalar(0, 200, 0), 3, 4);
                    Register_Event(key, 0);
                    key_event[key] = false;
                }
            }
        }
        else {
            // ����
            y = key_black_y;
            if (!key_event[key]) {
                // �����̐F���ς�����Ƃ�
                if (Change_Color_b(Get_Color_b(x, y), Get_Color_g(x, y), Get_Color_r(x, y))) {
                    cout << "[" << key << "B:on]" << endl;
                    cv::circle(frame, cv::Point(x, y), 3, cv::Scalar(0, 0, 200), 3, 4);
                    Register_Event(key, 1);
                    key_event[key] = true;
                }
            }
            else {
                // �����̐F�����ɖ߂����Ƃ�
                if (!Change_Color_b(Get_Color_b(x, y), Get_Color_g(x, y), Get_Color_r(x, y))) {
                    cout << "[" << key << "B:off]" << endl;
                    cv::circle(frame, cv::Point(x, y), 3, cv::Scalar(0, 200, 0), 3, 4);
                    Register_Event(key, 0);
                    key_event[key] = false;
                }
            }
        }
    }
}

// �w�肳�ꂽ���W�̐F�����擾����֐�
int Analysis::Get_Color_b(int x, int y) {
    return frame.at<Vec3b>(y, x)[0];
}

// �w�肳�ꂽ���W�̗ΐF�����擾����֐�
int Analysis::Get_Color_g(int x, int y) {
    return frame.at<Vec3b>(y, x)[1];
}

// �w�肳�ꂽ���W�̐ԐF�����擾����֐�
int Analysis::Get_Color_r(int x, int y) {
    return frame.at<Vec3b>(y, x)[2];
}

// �^����ꂽ�L�[�ԍ����������ǂ����𔻒肷��֐�
bool Analysis::True_White(int n) {
    n += 9;
    int a = n % 12;
    switch (a) {
    case 0: return true;   // �h
    case 1: return false;  // #�h
    case 2: return true;   // ��
    case 3: return false;  // #��
    case 4: return true;   // �~
    case 5: return true;   // �t�@
    case 6: return false;  // #�t�@
    case 7: return true;   // �\
    case 8: return false;  // #�\
    case 9: return true;   // ��
    case 10: return false; // #��
    case 11: return true;  // �V

    default: break;
    }
}

// �C�x���g��o�^����֐�
void Analysis::Register_Event(int key, int event) {
    std::ostringstream key_;
    key_ << key;

    str += "-";
    str += to_string(event);
    str += key_.str();
    active_key_sum++;
}

// ��͌��ʂ��e�L�X�g�t�@�C���ɏ����o���֐�
void Analysis::Output_txt() {
    ofstream outputfile("output.txt");
    outputfile << str_;
    outputfile.close();
}

int main() {
    Analysis analysis; // Analysis�N���X�̃C���X�^���X�𐶐�
    analysis.Check_Coodinates(); // ���Ղ̍��W���m�F����֐����Ăяo��
    analysis.Analyze(); // �f������͂��ăC�x���g�����m���A���ʂ��e�L�X�g�t�@�C���ɏ����o���֐����Ăяo��
}