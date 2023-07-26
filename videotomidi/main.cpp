#include <opencv2/opencv.hpp>

using namespace cv;
using namespace std;

class Movie {
private:
    VideoCapture cap; // 動画を読み込むためのバッファ
    Mat img; // 画像用のバッファ、動画から１フレームずつ取り出す
    int max_frame; // 動画の総フレーム数
    int cur_frame_num = 0; // 現在読み込んでいるフレーム

public:
    Movie() : cap("C:\\opencv\\video.mp4"), max_frame(cap.get(CAP_PROP_FRAME_COUNT)) {
        // オフセット分のフレームを飛ばす
        for (int i = 0; i < Get_FPS() * 3; i++) if (cur_frame_num < max_frame) cap >> img;
    }
    Mat Get_Next_Frame() { if (cur_frame_num < max_frame) cap >> img; return img; } // 次のフレームを取得する関数
    int Get_FPS() { return cap.get(CAP_PROP_FPS); } // 動画のフレームレートを取得する関数
    int Get_Max_Frame() { return max_frame; } // 動画の総フレーム数を取得する関数
    int Cur_Frame_Num() { return cur_frame_num; } // 現在のフレーム番号を取得する関数
};

class Analysis {
private:
    const static int threshold = 50;// 色がどれくらい変わったら変化したと認識するかの閾値
    Movie movie; // 動画の処理を行うインスタンス
    cv::Mat frame; // フレームを保持するMatオブジェクト

    // 座標の保持
    // 白鍵
    int key_white_y; // 縦軸
    int key_white_x[52]; // 横軸
    // 黒鍵
    int key_black_y; // 縦軸
    int key_black_x[36]; // 横軸
    // 88鍵盤全体
    int key_x[88];

    int active_key_sum = 0; // 画面変更の時の検知対策(変化量が多すぎると、書き出ししないフラグみたいな使い方)
    int first_key = 0; // 初めてのキーイベントのフラグ

    // 各鍵盤が押されてない初期状態のRGB値を保持する変数
    // 白鍵
    int def_w_clrB, def_w_clrG, def_w_clrR;
    // 黒鍵
    int def_b_clrB, def_b_clrG, def_b_clrR;

    // 各鍵盤の座標のイベント状態（押されたかどうか）を保持する配列
    bool key_w_event[52] = { false }; // 白鍵
    bool key_b_event[36] = { false }; // 黒鍵
    bool key_event[88] = { false }; // 88鍵盤全体のイベント状態を保持する配列

    string str = ""; // 1フレーム分を保持する文字列
    string str_ = ""; // その1フレーム分に問題がなければ、書き込み


public:
    Analysis(); // コンストラクタ
    void Set_Coodinates(); // 鍵盤の座標を設定する関数
    void Set_Color(); // 鍵盤が押されていない初期状態のRGBを記録する関数
    void Analyze(); // 映像の解析を行い、txtファイルを生成する関数
    void Check_Coodinates(); // 画面の座標をチェックする関数

    // 各鍵盤の色が変わったかチェックする関数(色が変わっていたらtrueを返す)
    bool Change_Color_w(int b, int g, int r);// 白鍵
    bool Change_Color_b(int b, int g, int r);// 黒鍵

    void Check_Key(); // 各鍵盤の状態をチェックする関数

    // 指定された座標の情報を取得する関数
    int Get_Color_b(int x, int y); // 青色
    int Get_Color_g(int x, int y); // 緑色
    int Get_Color_r(int x, int y); // 赤色

    bool True_White(int n); // 鍵盤が白鍵かどうかを判定する関数
    void Register_Event(int key, int event); // 各鍵盤のイベント（押されたか離されたか）を記録する関数
    void Output_txt(); // 解析結果をテキストファイルに書き出す関数
};

// Analysisクラスのコンストラクタ
Analysis::Analysis() {
    Set_Coodinates(); // 鍵盤の座標を設定する関数の呼び出し
    Set_Color(); // 鍵盤が押されていない状態のRGB値を記録する関数の呼び出し
}

// 鍵盤の座標を設定する関数
void Analysis::Set_Coodinates() {
    cout << "Set_Coodinates() 座標セット関数" << endl;

    // 720p 88鍵盤 の座標設定
    cout << "Mode = 720p 88 key" << endl;

    // 白鍵
    // Y座標(鍵盤は、横に並んでいるので縦軸は変更なし)
    key_white_y = 665;
    // X座標を計算
    for (int i = 0; i < 52; i++) key_white_x[i] = (24.5 / 2.0) + i * 24.6;

    // 黒鍵
    // Y座標(鍵盤は、横に並んでいるので縦軸は変更なし)
    key_black_y = 620;
    // X座標を計算
    key_black_x[0] = 28.5; // 一番最初の黒鍵座標設定
    int k = 0; // 1オクターブ分の黒鍵変数
    int def = 27; // 黒鍵座標変数
    double W_1 = 43.2; // 間に黒鍵がない
    double W_2 = 29.3; // 間に黒鍵がある
    for (int i = 1; i < 36; i++) {
        switch (k) {
        case 0: def += W_1; break; // #ド
        case 1: def += W_2; break; // #レ
        case 2: def += W_1; break; // #ファ
        case 3: def += W_2; break; // #ソ
        case 4: // #ラ
            def += W_2;
            k = -1;// 1オクターブ分完了 黒鍵変数リセット
            break;
        }
        k++; // 次の黒鍵
        key_black_x[i] = def; // 黒鍵座標を登録
    }

    // 88鍵盤全体の座標を保持する配列を設定
    int white = 0, black = 0;
    for (int num = 0; num < 88; num++) {
        if (True_White(num)) { // 白鍵か黒鍵かを判定し、対応するX座標を設定
            // 白鍵
            key_x[num] = key_white_x[white];
            white++;
            cout << num << "w";
        }
        else {
            // 黒鍵
            key_x[num] = key_black_x[black];
            black++;
            cout << num << "b";
        }
    }

    cout << "" << endl;

    // X座標を昇順にソート
    sort(key_x, key_x + 88);

    // 座標の確認を行う関数を呼び出し
    Check_Coodinates();
}

// 鍵盤が押されていない初期状態のRGB値を記録する関数
void Analysis::Set_Color() {
    cout << "Set_Color() デフォルトカラー取得関数" << endl;

    // 各鍵盤の初期状態のRGB値を記録
    // 白鍵
    def_w_clrB = frame.at<Vec3b>(key_white_y, key_white_x[0])[0];
    def_w_clrG = frame.at<Vec3b>(key_white_y, key_white_x[0])[1];
    def_w_clrR = frame.at<Vec3b>(key_white_y, key_white_x[0])[2];
    // 黒鍵
    def_b_clrB = frame.at<Vec3b>(key_black_y, key_black_x[0])[0];
    def_b_clrG = frame.at<Vec3b>(key_black_y, key_black_x[0])[1];
    def_b_clrR = frame.at<Vec3b>(key_black_y, key_black_x[0])[2];

    cout << "B:" << def_w_clrB << ",G:" << def_w_clrG << ",R:" << def_w_clrR << endl;
    cout << "B:" << def_b_clrB << ",G:" << def_b_clrG << ",R:" << def_b_clrR << endl;
}

// 映像の解析を行い、txtファイルを生成する関数
void Analysis::Analyze() {
    const static double fps = movie.Get_FPS();
    // 1フレームずつ処理する
    for (int frame_count = 1;; frame = movie.Get_Next_Frame()) {
        if (frame.empty()) break;

        Check_Key(); // キーのイベントをアップデート

        double time_now = (double)frame_count / fps; // 1フレームをfpsで割ることで、時間が出る

        // 同時発音数が一定以下でイベントを本登録
        if (str.size() > 0 && str.size() < 30) {
            cout << std::to_string(time_now) << endl;
            str_ += std::to_string(time_now) + "ms" + str + "\n";
        }
        str = "";
        frame_count++; // 処理が終わったのちに、次のフレームを追加する
        first_key = 0;
        active_key_sum = 0;
        cv::imshow("movie", frame);
    }
    Output_txt(); // 最後にファイルに書き出し
}

// 座標のチェックを行う関数
void Analysis::Check_Coodinates() {
    cout << "Check_Coodinates() 座標チェック関数" << endl;

    cv::namedWindow("movie", cv::WINDOW_AUTOSIZE);

    frame = movie.Get_Next_Frame();

    // 白鍵の座標を赤色の円で表示
    for (const auto& e : key_white_x) {
        cout << e << endl;
        cv::circle(frame, cv::Point(e, key_white_y), 3, cv::Scalar(0, 0, 200), 3, 4);
    }
    // 黒鍵の座標を緑色の円で表示
    for (const auto& e : key_black_x) {
        cout << e << endl;
        cv::circle(frame, cv::Point(e, key_black_y), 3, cv::Scalar(0, 200, 0), 3, 4);
    }
    int i = 0;
    // 88鍵盤全体の座標を黄色の円で表示
    for (const auto& e : key_x) {
        cout << i << "^" << e;
        cv::circle(frame, cv::Point(e, key_black_y + 30), 3, cv::Scalar(255, 255, 0), 3, 4);
        i += 1;
    }

    cv::imshow("movie", frame);

    cv::waitKey(0);
}

// 色が変わったかチェックする関数
// 白鍵
bool Analysis::Change_Color_w(int b, int g, int r) {
    int diff = abs(b - def_w_clrB) + abs(g - def_w_clrG) + abs(r - def_w_clrR);
    return diff > threshold;
}
// 黒鍵
bool Analysis::Change_Color_b(int b, int g, int r) {
    int diff = abs(b - def_b_clrB) + abs(g - def_b_clrG) + abs(r - def_b_clrR);
    return diff > threshold;
}

// キーのイベントをチェックする関数
void Analysis::Check_Key() {
    int x, y, key;

    for (key = 0; key < 88; key++) {
        x = key_x[key];
        if (True_White(key)) {
            // 白鍵
            y = key_white_y;
            if (!key_event[key]) {
                // 白鍵の色が変わったとき
                if (Change_Color_w(Get_Color_b(x, y), Get_Color_g(x, y), Get_Color_r(x, y))) {
                    cout << "[" << key << "W:on]" << endl;
                    cv::circle(frame, cv::Point(x, y), 3, cv::Scalar(0, 0, 200), 3, 4);
                    Register_Event(key, 1);
                    key_event[key] = true;
                }
            }
            else {
                // 白鍵の色が元に戻ったとき
                if (!Change_Color_w(Get_Color_b(x, y), Get_Color_g(x, y), Get_Color_r(x, y))) {
                    cout << "[" << key << "W:off]" << endl;
                    cv::circle(frame, cv::Point(x, y), 3, cv::Scalar(0, 200, 0), 3, 4);
                    Register_Event(key, 0);
                    key_event[key] = false;
                }
            }
        }
        else {
            // 黒鍵
            y = key_black_y;
            if (!key_event[key]) {
                // 黒鍵の色が変わったとき
                if (Change_Color_b(Get_Color_b(x, y), Get_Color_g(x, y), Get_Color_r(x, y))) {
                    cout << "[" << key << "B:on]" << endl;
                    cv::circle(frame, cv::Point(x, y), 3, cv::Scalar(0, 0, 200), 3, 4);
                    Register_Event(key, 1);
                    key_event[key] = true;
                }
            }
            else {
                // 黒鍵の色が元に戻ったとき
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

// 指定された座標の青色情報を取得する関数
int Analysis::Get_Color_b(int x, int y) {
    return frame.at<Vec3b>(y, x)[0];
}

// 指定された座標の緑色情報を取得する関数
int Analysis::Get_Color_g(int x, int y) {
    return frame.at<Vec3b>(y, x)[1];
}

// 指定された座標の赤色情報を取得する関数
int Analysis::Get_Color_r(int x, int y) {
    return frame.at<Vec3b>(y, x)[2];
}

// 与えられたキー番号が白鍵かどうかを判定する関数
bool Analysis::True_White(int n) {
    n += 9;
    int a = n % 12;
    switch (a) {
    case 0: return true;   // ド
    case 1: return false;  // #ド
    case 2: return true;   // レ
    case 3: return false;  // #レ
    case 4: return true;   // ミ
    case 5: return true;   // ファ
    case 6: return false;  // #ファ
    case 7: return true;   // ソ
    case 8: return false;  // #ソ
    case 9: return true;   // ラ
    case 10: return false; // #ラ
    case 11: return true;  // シ

    default: break;
    }
}

// イベントを登録する関数
void Analysis::Register_Event(int key, int event) {
    std::ostringstream key_;
    key_ << key;

    str += "-";
    str += to_string(event);
    str += key_.str();
    active_key_sum++;
}

// 解析結果をテキストファイルに書き出す関数
void Analysis::Output_txt() {
    ofstream outputfile("output.txt");
    outputfile << str_;
    outputfile.close();
}

int main() {
    Analysis analysis; // Analysisクラスのインスタンスを生成
    analysis.Check_Coodinates(); // 鍵盤の座標を確認する関数を呼び出す
    analysis.Analyze(); // 映像を解析してイベントを検知し、結果をテキストファイルに書き出す関数を呼び出す
}