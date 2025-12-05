#pragma once

#include <string>

class LoginScreen {
public:
    LoginScreen();
    ~LoginScreen();
    
    void render();
    
    bool isLoginSuccessful() const;
    std::string getUserId() const;
    
    void reset();
    
private:
    char userIdBuffer[256];
    bool loginSuccessful;
    std::string errorMessage;
    bool connecting;
};