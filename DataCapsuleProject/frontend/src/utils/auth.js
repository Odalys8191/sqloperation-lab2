const SESSION_DURATION_MS = 24 * 60 * 60 * 1000;

export function isAuthenticated() {
    const token = localStorage.getItem('token');
    const loginTime = localStorage.getItem('login_time');
    if (!token || !loginTime) {
        return false;
    }
    return Date.now() - parseInt(loginTime, 10) < SESSION_DURATION_MS;
}

export function authHeaders() {
    const headers = { 'Content-Type': 'application/json' };
    const token = localStorage.getItem('token');
    if (token) {
        headers.Authorization = `Bearer ${token}`;
    }
    return headers;
}

export function clearAuthState() {
    localStorage.removeItem('token');
    localStorage.removeItem('refresh');
    localStorage.removeItem('user_id');
    localStorage.removeItem('password');
    localStorage.removeItem('login_time');
}

export function requireAuthBeforeAction(actionName = '执行该操作') {
    if (isAuthenticated()) {
        return true;
    }
    clearAuthState();
    alert(`请先登录后再${actionName}。`);
    return false;
}

export async function validateServerSession() {
    if (!isAuthenticated()) {
        clearAuthState();
        return false;
    }

    try {
        const response = await fetch('/api/consumer/consumeDataCapsule', {
            method: 'POST',
            headers: authHeaders(),
            body: JSON.stringify({})
        });

        if (response.status === 401) {
            clearAuthState();
            return false;
        }

        return true;
    } catch (error) {
        // 网络抖动时不立即清空本地登录态，避免误登出
        return true;
    }
}

export async function requireValidSessionBeforeAction(actionName = '执行该操作') {
    const isSessionValid = await validateServerSession();
    if (isSessionValid) {
        return true;
    }
    alert(`登录状态已失效，请重新登录后再${actionName}。`);
    return false;
}
