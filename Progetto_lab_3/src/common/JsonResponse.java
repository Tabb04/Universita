package common;

import com.google.gson.JsonObject;

/**
 * Mappa una risposta JSON dal Server al Client.
 */
public class JsonResponse {
    public String status; // "success" o "error"
    public int errorCode; // 0 se success
    public String errorMessage; // Messaggio di errore, o null
    public JsonObject data; // Payload dinamico opzionale

    public JsonResponse(String status, int errorCode, String errorMessage, JsonObject data) {
        this.status = status;
        this.errorCode = errorCode;
        this.errorMessage = errorMessage;
        this.data = data;
    }
    
    public static JsonResponse success(JsonObject data) {
        return new JsonResponse("success", 0, null, data);
    }
    
    public static JsonResponse error(int errorCode, String errorMessage) {
        return new JsonResponse("error", errorCode, errorMessage, null);
    }
}
