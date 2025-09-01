package com.zjy.nerlargemodel.controller;

import com.zjy.nerlargemodel.aiservice.NERService;
import com.zjy.nerlargemodel.config.CosyVoiceSocketClient;
import com.zjy.nerlargemodel.entity.VoiceInputRequest;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.core.io.FileSystemResource;
import org.springframework.http.HttpEntity;
import org.springframework.http.HttpHeaders;
import org.springframework.http.ResponseEntity;
import org.springframework.util.LinkedMultiValueMap;
import org.springframework.util.MultiValueMap;
import org.springframework.web.bind.annotation.PostMapping;
import org.springframework.web.bind.annotation.RequestBody;
import org.springframework.web.bind.annotation.RequestParam;
import org.springframework.web.bind.annotation.RequestPart;
import org.springframework.web.bind.annotation.RestController;
import org.springframework.web.client.RestTemplate;
import org.springframework.web.multipart.MultipartFile;
import reactor.core.publisher.Flux;
import org.springframework.http.MediaType;

import java.io.File;
import java.io.IOException;
import java.net.URI;
import java.time.LocalDateTime;
import java.time.format.DateTimeFormatter;
import java.util.UUID;


@RestController
public class NerController {
    @Autowired
    private NERService nerService;


    @PostMapping("/ner")
    public Flux<String> recognizeEntities(@RequestParam("memoryId") String memoryId,
                                          @RequestBody(required = false) VoiceInputRequest voiceInputRequest,
                                          @RequestPart(value = "imageFile", required = false) MultipartFile imageFile) throws InterruptedException, IOException {
        
        String message = "";
        String userInfo = "";
        System.out.println(voiceInputRequest);
        // 处理语音输入请求
        if (voiceInputRequest != null) {
            message = voiceInputRequest.getTranscription();
            
            // 提取目标用户信息（只处理第一个用户）
            if (voiceInputRequest.getMentionedPersons() != null && !voiceInputRequest.getMentionedPersons().isEmpty()) {
                VoiceInputRequest.MentionedPerson targetUser = voiceInputRequest.getMentionedPersons().get(0);
                
                userInfo = "目标用户ID：" + targetUser.getId() + 
                          "\n目标用户信息：" + targetUser.getName() + "(" + targetUser.getRelationType() + ")\n";
                System.out.println(userInfo);
            }
            
            // 添加说话人信息
            if (voiceInputRequest.getSpeaker() != null) {
                userInfo += "说话人：" + voiceInputRequest.getSpeaker().getName() + 
                           " (ID: " + voiceInputRequest.getSpeaker().getId() + ")\n";
            }
        }
        
        // 1.处理图片，调用 OCR 服务
        if (imageFile != null && !imageFile.isEmpty()) {
            String ocrText = sendImageToOCRService(imageFile);
            message = (message == null ? "" : message + "\n") + "【以下是药品照片上的文字】：" + ocrText;
        }

        String currentTime = LocalDateTime.now().format(DateTimeFormatter.ofPattern("yyyy-MM-dd HH:mm:ss"));
        String fullPrompt = "当前时间：" + currentTime + "\n" + userInfo + "用户输入：" + message;
        System.out.println(fullPrompt);
        URI uri = URI.create("ws://localhost:8082");
        String sessionId = UUID.randomUUID().toString();
        CosyVoiceSocketClient ttsSocket = new CosyVoiceSocketClient(uri, sessionId);
        ttsSocket.connectBlocking();  // 等待连接成功
        Flux<String> response = nerService.recognizeEntities(memoryId, fullPrompt);

        return response
                .map(chunk -> chunk.replaceAll("[*#]", "")) // 🔧 删除所有星号
                .doOnNext(chunk -> {
                    System.out.println("📤 向语音服务发送分片: " + chunk);
                    ttsSocket.sendTextChunk(chunk);
                })
                .doOnComplete(() -> {
                    try {
                        ttsSocket.sendSessionEnd();
                    } catch (Exception e) {
                        System.out.println("⚠️ 发送session_end异常: " + e.getMessage());
                    } finally {
                        try { ttsSocket.close(); } catch (Exception ignore) {}
                    }
                });
    }


    private String sendImageToOCRService(MultipartFile imageFile) throws IOException {
        String targetUrl = "http://81.69.221.200:8081/ocr";  // 替换为你的服务地址

        HttpHeaders headers = new HttpHeaders();
        headers.setContentType(MediaType.MULTIPART_FORM_DATA);

        // 封装文件
        MultiValueMap<String, Object> body = new LinkedMultiValueMap<>();
        File tempFile = File.createTempFile("upload", ".jpg");
        imageFile.transferTo(tempFile);
        body.add("file", new FileSystemResource(tempFile));

        HttpEntity<MultiValueMap<String, Object>> requestEntity = new HttpEntity<>(body, headers);

        RestTemplate restTemplate = new RestTemplate();
        ResponseEntity<String> response = restTemplate.postForEntity(targetUrl, requestEntity, String.class);

        return response.getBody();  // 假设 OCR 服务返回提取的文字
    }

}
