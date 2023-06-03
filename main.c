#include <windows.h>
#include "exec.h"


#define SCREEN_W 500
#define SCREEN_H 500
#define SCREEN_GRID N_GRID
#define SCREEN_BTN_W SCREEN_W / SCREEN_GRID
#define SCREEN_BTN_H SCREEN_H / SCREEN_GRID

#define SCREEN_DRAW 0xFF
#define SCREEN_RESET 0xF0



DWORD WINAPI threadRoutine(LPVOID lpParam) {

  printf("\n\n Reseau DeepL à %d x %d : N-to-N \n", LINES, COLUMNS);
  printf("\n\t Neuron a visualisé\n");
  printf("\n\t x: ");
  scanf("%d", &posNeuronOut.i);
  printf("\t y: ");
  scanf("%d", &posNeuronOut.j);
  printf("\n\t Le nombre dessiné: ");
  scanf("%d", &numberLook);
  printf("\n\t Sortie souhaitee: ");
  scanf("%d", &ref_out);
  printf("\n\t Rate learning: ");
  scanf("%Lf", &ref_learn);
  printf("\n\t Iteration de calibration: ");
  scanf("%d", &max_count);
  printf("\n\t Parametres aleatoires: 0/1 >>> ");
  scanf("%d", &okGenerate);

  if (posNeuronOut.i < 0)
    posNeuronOut.i = 0;
  else if (posNeuronOut.i > LINES - 1)
    posNeuronOut.i = LINES - 1;
  if (posNeuronOut.j < 0)
    posNeuronOut.j = 0;
  else if (posNeuronOut.j > COLUMNS - 1)
    posNeuronOut.j = COLUMNS - 1;

  printf("\n\t Calibration du neurone [ %d - %d ] \n", posNeuronOut.i, posNeuronOut.j);
  printf("\n\t ............ \n");

  return 0;
}


typedef struct {
  HWND btn;
  BOOL state;
} GRID;
GRID grid[SCREEN_GRID][SCREEN_GRID];

typedef struct {
  HANDLE handleThread;
  DWORD state;
} WinThread;

LRESULT CALLBACK hWndEvent(HWND hWnd, UINT wInMsg, WPARAM wParam, LPARAM lParam) {

  static WinThread winThread = { NULL, -1 };
  HBRUSH hbrWhite = CreateSolidBrush(RGB(255, 255, 255));
  HBRUSH hbrBlack = CreateSolidBrush(RGB(0, 0, 0));

  switch (wInMsg)
  {

  case WM_KEYDOWN:
    if (wParam == VK_RETURN) {
      bool vect[SCREEN_GRID * SCREEN_GRID];
      for (int i = 0, k = 0; i < SCREEN_GRID; i++)
        for (int j = 0; j < SCREEN_GRID; j++, k++)
          vect[k] = grid[j][i].state; // inverse sens m x n

      exec(vect);
    }
    if (wParam == VK_SPACE) {
      bool vect[SCREEN_GRID * SCREEN_GRID];
      for (int i = 0, k = 0; i < SCREEN_GRID; i++)
        for (int j = 0; j < SCREEN_GRID; j++, k++)
          vect[k] = grid[j][i].state; // inverse sens m x n

      if (winThread.state == WAIT_OBJECT_0)
        CloseHandle(winThread.handleThread);
      else if (winThread.state == WAIT_TIMEOUT)
      {
        TerminateThread(winThread.handleThread, 0);
        CloseHandle(winThread.handleThread);
      }
      winThread.handleThread = CreateThread(NULL, 0, threadRoutine, winThread.handleThread, 0, NULL);
      winThread.state = WaitForSingleObject(winThread.handleThread, 1200000);
      paramConfigDebug(vect, posNeuronOut, ref_out, ref_learn, max_count, okGenerate);
    }

    if (wParam == VK_DELETE || wParam == VK_BACK)
      SendMessageA(hWnd, SCREEN_RESET, -1, -1);
    if (wParam == VK_ESCAPE)
      DestroyWindow(hWnd);
    break;

  case WM_CTLCOLORBTN:
    return (LRESULT)hbrWhite;
    break;

  case WM_SETCURSOR:
    if (LOWORD(lParam) == HTCLIENT && GetAsyncKeyState(VK_LBUTTON) & 0x8000) {
      for (int i = 0; i < SCREEN_GRID; i++)
        for (int j = 0; j < SCREEN_GRID; j++)
          if (grid[i][j].btn == (HWND)wParam) {
            if (grid[i][j].state)
              break;
            grid[i][j].state = TRUE;
            SendMessageA(hWnd, SCREEN_DRAW, (WPARAM)grid[i][j].btn, (LPARAM)TRUE);
          }
    }
    SetCursor(LoadCursorA(NULL, IDC_HAND));
    SetFocus(hWnd);
    break;

  case SCREEN_DRAW:
  {
    HDC hdc = GetDC((HWND)wParam);
    RECT rect;
    GetClientRect((HWND)wParam, &rect);
    FillRect(hdc, &rect, (BOOL)lParam ? hbrBlack : hbrWhite);
    ReleaseDC((HWND)wParam, hdc);
  }
  break;

  case SCREEN_RESET:
    for (int i = 0; i < SCREEN_GRID; i++)
      for (int j = 0; j < SCREEN_GRID; j++) {
        grid[i][j].state = FALSE;
        SendMessageA(hWnd, SCREEN_DRAW, (WPARAM)grid[i][j].btn, (LPARAM)FALSE);
      }
    break;

  case WM_CREATE:

    printf("\n\tAppuie sur ESC >< quit");
    printf("\n\tAppuie sur ESPACE >< Configuration & calibrage de parametres deepL");
    printf("\n\tAppuie sur ENTREE >< Lancer le test");
    printf("\n\tAppuie sur ENTREE >< 2 cliquez sur la souris pour dessiner\n");


    for (int i = 0; i < SCREEN_GRID; i++)
      for (int j = 0; j < SCREEN_GRID; j++) {
        grid[i][j].btn = CreateWindowEx(
          WS_EX_OVERLAPPEDWINDOW,
          "BUTTON",
          NULL,
          WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
          SCREEN_BTN_W * i, SCREEN_BTN_H * j,
          SCREEN_BTN_W, SCREEN_BTN_H,
          hWnd,
          NULL,
          (HINSTANCE)GetWindowLongPtrA(hWnd, GWLP_HINSTANCE),
          NULL);
        grid[i][j].state = FALSE;
      }

    break;

  case WM_DESTROY:
    CloseHandle(winThread.handleThread);
    DeleteObject(hbrWhite);
    DeleteObject(hbrBlack);
    PostQuitMessage(0);
    break;

  default:
    return DefWindowProcA(hWnd, wInMsg, wParam, lParam);
  }
}

void loop() {
  MSG msg;
  BOOL ok;
  while (ok = GetMessage(&msg, NULL, 0, 0) != 0)
    if (ok != -1) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
}


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {

  WNDCLASSEX wc;
  wc.cbSize = sizeof(WNDCLASSEX);
  wc.style = CS_BYTEALIGNWINDOW;
  wc.lpfnWndProc = hWndEvent;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;
  wc.hInstance = hInstance;
  wc.hIcon = LoadIconA(NULL, IDI_SHIELD);
  wc.hCursor = LoadCursorA(NULL, IDC_CROSS);
  wc.hbrBackground = CreateSolidBrush(RGB(0, 0, 0));
  wc.lpszMenuName = NULL;
  wc.lpszClassName = "WC";
  wc.hIconSm = LoadIconA(NULL, IDI_SHIELD);

  if (!RegisterClassEx(&wc))
    return -1;


  HWND hWnd = CreateWindowEx(
    WS_EX_OVERLAPPEDWINDOW,
    "WC",
    "img@recognition",
    WS_POPUP | WS_VISIBLE,
    (GetSystemMetrics(SM_CXSCREEN) - SCREEN_W) / 8, (GetSystemMetrics(SM_CYSCREEN) - SCREEN_H) / 2,
    SCREEN_W, SCREEN_H,
    NULL,
    NULL,
    hInstance,
    NULL
  );

  if (hWnd != NULL)
    ShowWindow(hWnd, SW_SHOW);

  loop();

  return 0;
}
