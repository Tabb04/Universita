package common;

import com.google.gson.JsonObject;


//Mappo risposta Json dal server al client

public class JsonResponse{
    public String status;           //Indica se success o error
    public int errorCode;           //Se è success allora 0
    public String errorMessage;     //Se success è null
    public JsonObject data;         //payload opzionale


    public JsonResponse(String status, int errorCode, String errorMessage, JsonObject data){
        this.status = status;
        this.errorCode = errorCode;
        this.errorMessage = errorMessage;
        this.data = data;
    }
    
    public static JsonResponse success(JsonObject data){
        return new JsonResponse("success", 0, null, data);
    }
    
    public static JsonResponse error(int errorCode, String errorMessage){
        return new JsonResponse("error", errorCode, errorMessage, null);
    }
}
